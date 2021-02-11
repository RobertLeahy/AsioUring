#include <asio_uring/execution_context.hpp>

#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <thread>
#include <asio_uring/liburing.hpp>
#include <errno.h>
#include <poll.h>

#include <iostream>

namespace asio_uring {

namespace {

template<typename T>
std::uint64_t to_user_data(const T& t) noexcept {
  const T* ptr = std::addressof(t);
  static_assert(sizeof(std::uint64_t) == sizeof(ptr));
  std::uint64_t retr;
  std::memcpy(&retr,
              &ptr,
              sizeof(retr));
  return retr;
}

template<typename T>
T& from_user_data(std::uint64_t user_data) noexcept {
  static_assert(sizeof(user_data) == sizeof(T*));
  T* ptr;
  std::memcpy(&ptr,
              &user_data,
              sizeof(ptr));
  assert(ptr);
  return *ptr;
}

enum class error {
  success = 0,
  no_sqe_for_eventfd,
  no_sqe
};

std::error_code make_error_code(error err) noexcept {
  static const class : public std::error_category {
  public:
    virtual const char* name() const noexcept override {
      return "io_uring execution_context";
    }
    virtual std::string message(int code) const override {
      switch (static_cast<error>(code)) {
      case error::success:
        return "Success";
      case error::no_sqe_for_eventfd:
        return "No submission queue entry to enqueue operation against internal event fd";
      case error::no_sqe:
        return "No submission queue entry";
      default:
        break;
      }
      return "Unknown";
    }
    virtual std::error_condition default_error_condition(int code) const noexcept override {
      switch (static_cast<error>(code)) {
      case error::success:
        return std::error_condition();
      case error::no_sqe_for_eventfd:
      case error::no_sqe:
        return std::error_code(EBUSY,
                               std::generic_category()).default_error_condition();
      default:
        break;
      }
      return std::error_condition(code,
                                  *this);
    }
  } category;
  return std::error_code(static_cast<int>(err),
                         category);
}

[[noreturn]]
void throw_error_code(error err) {
  throw std::system_error(make_error_code(err));
}

}

execution_context::executor_type::executor_type(execution_context& ctx) noexcept
  : ctx_(&ctx)
{}

execution_context& execution_context::executor_type::context() const noexcept {
  assert(ctx_);
  return *ctx_;
}

void execution_context::executor_type::on_work_started() const noexcept {
  assert(ctx_);
  ctx_->work_.fetch_add(1,
                        std::memory_order_release);
}

void execution_context::executor_type::on_work_finished() const noexcept {
  assert(ctx_);
  std::size_t rem = ctx_->work_.fetch_sub(1,
                                          std::memory_order_release);
  assert(rem);
  --rem;
  if (!rem) {
    ctx_->zero_.write(1);
  }
}

bool execution_context::executor_type::operator==(const executor_type& rhs) const noexcept {
  assert(ctx_);
  assert(rhs.ctx_);
  return ctx_ == rhs.ctx_;
}

bool execution_context::executor_type::operator!=(const executor_type& rhs) const noexcept {
  return !(*this == rhs);
}

execution_context::execution_context(unsigned entries,
                                     unsigned flags)
  : q_started_   (false),
    pending_     (0),
    stop_started_(false),
    zero_started_(false),
    work_        (0),
    stopped_     (false),
    u_           (entries,
                  flags)
{
  int arr[3];
  arr[0] = q_.native_handle();
  arr[1] = stop_.native_handle();
  arr[2] = zero_.native_handle();
  if (::io_uring_register_files(reinterpret_cast<io_uring*>(u_.native_handle()),
                          arr,
                          3))
  {
    std::error_code ec(errno,
                       std::generic_category());
    throw std::system_error(ec);
  }
  restart();
}

execution_context::executor_type execution_context::get_executor() noexcept {
  return executor_type(*this);
}

void execution_context::restart() {
  restart_if(0,
             q_started_);
  restart_if(1,
             stop_started_);
  restart_if(2,
             zero_started_);
  stopped_.store(false,
                 std::memory_order_relaxed);
}

execution_context::count_type execution_context::run() {
  return all_impl<true>();
}

execution_context::count_type execution_context::run_one() {
  return one_impl<true>();
}

execution_context::count_type execution_context::poll() {
  return all_impl<false>();
}

execution_context::count_type execution_context::poll_one() {
  return one_impl<false>();
}

void execution_context::stop() noexcept {
  stopped_.store(true,
                 std::memory_order_release);
  stop_.write(1);
}

execution_context::native_handle_type execution_context::native_handle() noexcept {
  return u_.native_handle();
}

execution_context::const_native_handle_type execution_context::native_handle() const noexcept {
  return u_.native_handle();
}

bool execution_context::running_in_this_thread() const noexcept {
  return tid_.load(std::memory_order_relaxed) == std::this_thread::get_id();
}

bool execution_context::out_of_work() const noexcept {
  return !work_.load(std::memory_order_acquire);
}

::io_uring_sqe& execution_context::get_sqe() {
  auto sqe = ::io_uring_get_sqe(u_.native_handle());
  if (!sqe) {
    throw_error_code(error::no_sqe_for_eventfd);
  }
  return *sqe;
}

execution_context::tid_guard::tid_guard(std::atomic<std::thread::id>& tid) noexcept
  : tid_(tid)
{
#ifdef NDEBUG
  tid_.store(std::this_thread::get_id(),
             std::memory_order_relaxed);
#else
  assert(tid_.exchange(std::this_thread::get_id(),
                       std::memory_order_relaxed) == std::thread::id());
#endif
}

execution_context::tid_guard::~tid_guard() noexcept {
#ifdef NDEBUG
  tid_.store(std::thread::id(),
             std::memory_order_relaxed);
#else
  assert(tid_.exchange(std::thread::id(),
                       std::memory_order_relaxed) == std::this_thread::get_id());
#endif
}

execution_context::handle_cqe_type::handle_cqe_type() noexcept
  : handlers(0),
    stopped (false)
{}

template<bool Blocking>
execution_context::count_type execution_context::all_impl() {
  if (stopped() || out_of_work()) {
    stopped_.store(true,
                   std::memory_order_relaxed);
    return 0;
  }
  count_type retr = 0;
  tid_guard tid_g(tid_);
  for (;;) {
    assert(!stopped());
    auto tmp = pending_;
    auto done = service_queue(pending_);
    assert(!pending_);
    retr += tmp;
    if (done) {
      stopped_.store(true,
                     std::memory_order_relaxed);
      return retr;
    }
    assert(!stopped());
    auto result = impl<Blocking>();
    retr += result.handlers;
    if (result.stopped) {
      stopped_.store(true,
                     std::memory_order_relaxed);
      return retr;
    }
    restart_if(0,
               q_started_);
  }
}

template<bool Blocking>
execution_context::count_type execution_context::one_impl() {
  if (stopped() || out_of_work()) {
    stopped_.store(true,
                   std::memory_order_relaxed);
    return 0;
  }
  tid_guard tid_g(tid_);
  if (pending_) {
    service_queue(1);
    stopped_.store(true,
                   std::memory_order_relaxed);
    return 1;
  }
  auto handlers = impl<Blocking>().handlers;
  stopped_.store(true,
                 std::memory_order_relaxed);
  return handlers;
}

template<bool Blocking>
execution_context::handle_cqe_type execution_context::impl() {
  assert(!stopped());
  ::io_uring_cqe* cqe;
  int result;
  if constexpr (Blocking) {
    result = ::io_uring_wait_cqe(u_.native_handle(),
                                 &cqe);
  } else {
    result = ::io_uring_peek_cqe(u_.native_handle(),
                                 &cqe);
  }
  if (result < 0) {
    std::error_code ec(errno,
                       std::generic_category());
    throw std::system_error(ec);
  }
  if constexpr (!Blocking) {
    if (!cqe) {
      handle_cqe_type retr;
      retr.stopped = true;
      return retr;
    }
  }
  assert(cqe);
  return handle_cqe(*cqe);
}

bool execution_context::stopped() const noexcept {
  if (!(q_started_    &&
        stop_started_ &&
        zero_started_))
  {
    return true;
  }
  return stopped_.load(std::memory_order_acquire);
}

void execution_context::restart(std::size_t i,
                                bool& b)
{
  assert(!b);
  ::io_uring_sqe* sqe = ::io_uring_get_sqe(u_.native_handle());
  if (!sqe) {
    throw_error_code(error::no_sqe_for_eventfd);
  }
  ::io_uring_prep_poll_add(sqe,
                           i,
                           POLLIN);
  sqe->flags |= IOSQE_FIXED_FILE;
  sqe->user_data = to_user_data(b);
  if (::io_uring_submit(u_.native_handle()) < 0) {
    std::error_code ec(errno,
                       std::generic_category());
    throw std::system_error(ec);
  }
  b = true;
}

void execution_context::restart_if(std::size_t i,
                                   bool& b)
{
  if (b) {
    return;
  }
  restart(i,
          b);
  assert(b);
}

execution_context::handle_cqe_type execution_context::handle_cqe(::io_uring_cqe& cqe) {
  assert(!stopped());
  class guard {
  public:
    guard(native_handle_type handle,
          ::io_uring_cqe& cqe) noexcept
      : handle_(handle),
        cqe_   (cqe)
    {}
    guard(const guard&);
    guard(guard&&);
    guard& operator=(const guard&);
    guard& operator=(guard&&);
    ~guard() noexcept {
      ::io_uring_cqe_seen(handle_,
                          &cqe_);
    }
  private:
    native_handle_type handle_;
    ::io_uring_cqe&    cqe_;
  };
  guard g(native_handle(),
          cqe);
  handle_cqe_type retr;
  if (cqe.user_data == to_user_data(stop_started_)) {
    stop_started_ = false;
    assert(stopped());
    stop_.read();
    if (stopped_.load(std::memory_order_acquire)) {
      retr.stopped = true;
      return retr;
    }
    restart(1,
            stop_started_);
    return retr;
  }
  if (cqe.user_data == to_user_data(zero_started_)) {
    zero_started_ = false;
    assert(stopped());
    zero_.read();
    if (!work_.load(std::memory_order_acquire)) {
      retr.stopped = true;
      return retr;
    }
    restart(2,
            zero_started_);
    assert(!stopped());
    return retr;
  }
  if (cqe.user_data == to_user_data(q_started_)) {
    q_started_ = false;
    assert(stopped());
    auto pending = q_.pending();
    assert(pending);
    pending_ += pending;
    retr.stopped = service_queue(1);
    ++retr.handlers;
    return retr;
  }
  from_user_data<completion>(cqe.user_data).complete(cqe);
  ++retr.handlers;
  return retr;
}

bool execution_context::service_queue(queue_type::integer_type max) {
  assert(max <= pending_);
  bool retr = false;
  q_.consume(max,
             [&](auto&& func) { --pending_;
                                auto work = work_.fetch_sub(1,
                                                            std::memory_order_acquire);
                                assert(work);
                                if (work == 1) {
                                  retr = true;
                                }
                                func(); });
  return retr;
}

bool execution_context::service_queue() {
  return service_queue(pending_);
}

}
