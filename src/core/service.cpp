#include <asio_uring/service.hpp>

#include <cassert>
#include <optional>
#include <system_error>
#include <utility>
#include <asio_uring/liburing.hpp>
#include <boost/core/noncopyable.hpp>
#include <errno.h>

namespace asio_uring {

service::completion::completion(service& svc)
  : svc_    (svc)
{}

void service::completion::complete(const ::io_uring_cqe& cqe) {
  assert(service_.is_linked());
  release_guard g(svc_,
                  *this);
  assert(wrapped_);
  (*wrapped_)(cqe);
}

void service::completion::reset() noexcept {
  if (wrapped_) {
    wrapped_ = std::nullopt;
    svc_.context().get_executor().on_work_finished();
  }
}

service::release_guard::release_guard(service& self,
                                      completion& c) noexcept
  : self_(&self),
    c_   (c)
{}

service::release_guard::~release_guard() noexcept {
  if (self_) {
    self_->release(c_);
  }
}

void service::release_guard::release() noexcept {
  self_ = nullptr;
}

service::implementation_type::to_const_void_pointer::result_type service::implementation_type::to_const_void_pointer::operator()(argument_type arg) const noexcept {
  return &arg;
}

service::implementation_type::const_iterator service::implementation_type::begin() const noexcept {
  return const_iterator(list_.begin(),
                        to_const_void_pointer());
}

service::implementation_type::const_iterator service::implementation_type::end() const noexcept {
  return const_iterator(list_.end(),
                        to_const_void_pointer());
}

service::service(execution_context& ctx)
  : ctx_(ctx)
{}

service::~service() noexcept {
  destroy_list(free_);
  for (auto&& completion : in_use_) {
    completion.reset();
  }
  destroy_list(in_use_);
}

void service::shutdown() noexcept {
  for (auto&& completion : in_use_) {
    completion.reset();
  }
}

execution_context& service::context() const noexcept {
  return ctx_;
}

void service::construct(implementation_type& impl) {
  assert(impl.list_.empty());
}

void service::destroy(implementation_type& impl) noexcept {
  impl.list_.clear();
}

void service::move_construct(implementation_type& impl,
                             implementation_type& src) noexcept
{
  assert(impl.list_.empty());
  impl.list_ = std::move(src.list_);
}

void service::move_assign(implementation_type& impl,
                          service& svc,
                          implementation_type& src) noexcept
{
  assert(this == &svc);
  destroy(impl);
  move_construct(impl,
                 src);
}

service::completion& service::maybe_allocate() {
  if (free_.empty()) {
    return *new completion(*this);
  }
  auto&& retr = free_.front();
  free_.pop_front();
  return retr;
}

service::completion& service::acquire(implementation_type& impl) {
  auto&& retr = maybe_allocate();
  assert(!retr.implementation_.is_linked());
  assert(!retr.service_.is_linked());
  in_use_.push_front(retr);
  impl.list_.push_front(retr);
  return retr;
}

service::iovs_type service::acquire(iovs_type::size_type s) {
  if (iovs_cache_.empty()) {
    iovs_type retr;
    retr.resize(s);
    return retr;
  }
  iovs_type retr(std::move(iovs_cache_.back()));
  assert(retr.empty());
  iovs_cache_.pop_back();
  return retr;
}

void service::release(completion& c) noexcept {
  assert(c.service_.is_linked());
  c.reset();
  release(c.iovs_);
  c.implementation_.unlink();
  c.service_.unlink();
  free_.push_front(c);
}

void service::release(iovs_type& iovs) noexcept {
  if (!iovs.capacity()) {
    return;
  }
  iovs.clear();
  try {
    iovs_cache_.emplace_back();
    using std::swap;
    swap(iovs,
         iovs_cache_.back());
  } catch (...) {}
}

void service::submit(::io_uring_sqe& sqe,
                     completion& c)
{
  ::io_uring_sqe_set_data(&sqe,
                          &c);
  auto result = ::io_uring_submit(ctx_.native_handle());
  if (result < 0) {
    std::error_code ec(errno,
                       std::generic_category());
    throw std::system_error(ec);
  }
}

void service::destroy_list(list_type& list) noexcept {
  while (!list.empty()) {
    auto&& front = list.front();
    list.pop_front();
    delete &front;
  }
}

}
