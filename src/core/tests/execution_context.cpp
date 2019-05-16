#include <asio_uring/execution_context.hpp>

#include <atomic>
#include <memory>
#include <optional>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>
#include <asio_uring/liburing.hpp>
#include <boost/asio/post.hpp>
#include <errno.h>

#include <catch2/catch.hpp>

namespace asio_uring::tests {
namespace {

TEST_CASE("execution_context stop",
          "[execution_context]")
{
  execution_context ctx(100);
  ctx.stop();
  auto handlers = ctx.run();
  CHECK(handlers == 0);
  handlers = ctx.run();
  CHECK(handlers == 0);
  ctx.restart();
  ctx.stop();
  handlers = ctx.run();
  CHECK(handlers == 0);
}

TEST_CASE("execution_context running_in_this_thread",
          "[execution_context]")
{
  execution_context ctx(100);
  CHECK_FALSE(ctx.running_in_this_thread());
}

TEST_CASE("execution_context executor_type on_work_started/_finished",
          "[execution_context]")
{
  execution_context ctx(100);
  ctx.get_executor().on_work_started();
  ctx.get_executor().on_work_started();
  std::atomic<bool> started(false);
  std::optional<execution_context::count_type> count;
  std::thread t([&]() {
    started = true;
    count = ctx.run();
  });
  while (!started);
  std::this_thread::yield();
  ctx.get_executor().on_work_finished();
  ctx.get_executor().on_work_finished();
  t.join();
  REQUIRE(count);
  CHECK(count == 0);
}

TEST_CASE("execution_context executor_type dispatch",
          "[execution_context]")
{
  execution_context ctx(100);
  std::allocator<void> a;
  bool invoked = false;
  ctx.get_executor().dispatch([&]() { invoked = true; },
                              a);
  CHECK_FALSE(invoked);
  auto handlers = ctx.run_one();
  CHECK(handlers == 1);
  CHECK(invoked);
  invoked = false;
  bool inner_invoked = false;
  auto func = [&]() {
    invoked = true;
    ctx.get_executor().dispatch([&]() { inner_invoked = true; },
                                a);
    CHECK(inner_invoked);
  };
  ctx.get_executor().dispatch(func,
                              a);
  CHECK_FALSE(invoked);
  handlers = ctx.run_one();
  CHECK(handlers == 0);
  CHECK_FALSE(invoked);
  ctx.restart();
  handlers = ctx.run_one();
  CHECK(handlers == 1);
  CHECK(invoked);
}

TEST_CASE("execution_context executor_type defer",
          "[execution_context]")
{
  execution_context ctx(100);
  std::allocator<void> a;
  bool invoked = false;
  ctx.get_executor().defer([&]() { invoked = true; },
                           a);
  CHECK_FALSE(invoked);
  auto handlers = ctx.run_one();
  CHECK(handlers == 1);
  CHECK(invoked);
  invoked = false;
  bool inner_invoked = false;
  auto func = [&]() {
    invoked = true;
    ctx.get_executor().defer([&]() { inner_invoked = true; },
                             a);
  };
  ctx.get_executor().defer(func,
                           a);
  CHECK_FALSE(invoked);
  handlers = ctx.run_one();
  CHECK(handlers == 0);
  CHECK_FALSE(invoked);
  ctx.restart();
  handlers = ctx.run_one();
  CHECK(handlers == 1);
  CHECK(invoked);
  CHECK_FALSE(inner_invoked);
  handlers = ctx.run_one();
  CHECK(handlers == 0);
  ctx.restart();
  handlers = ctx.run_one();
  CHECK(handlers == 1);
  CHECK(inner_invoked);
}

TEST_CASE("execution_context executor_type post",
          "[execution_context]")
{
  execution_context ctx(100);
  std::allocator<void> a;
  bool invoked = false;
  ctx.get_executor().post([&]() { invoked = true; },
                          a);
  CHECK_FALSE(invoked);
  auto handlers = ctx.run_one();
  CHECK(handlers == 1);
  CHECK(invoked);
  invoked = false;
  bool inner_invoked = false;
  auto func = [&]() {
    invoked = true;
    ctx.get_executor().post([&]() { inner_invoked = true; },
                            a);
  };
  ctx.get_executor().post(func,
                          a);
  CHECK_FALSE(invoked);
  handlers = ctx.run_one();
  CHECK(handlers == 0);
  CHECK_FALSE(invoked);
  ctx.restart();
  handlers = ctx.run_one();
  CHECK(handlers == 1);
  CHECK(invoked);
  CHECK_FALSE(inner_invoked);
  handlers = ctx.run_one();
  CHECK(handlers == 0);
  ctx.restart();
  handlers = ctx.run_one();
  CHECK(handlers == 1);
  CHECK(inner_invoked);
}

TEST_CASE("execution_context executor_type post move only",
          "[execution_context]")
{
  execution_context ctx(100);
  std::allocator<void> a;
  bool invoked = false;
  auto func = [ptr = std::make_unique<bool*>(&invoked)]() { **ptr = true; };
  static_assert(!std::is_copy_constructible_v<decltype(func)>);
  ctx.get_executor().post(std::move(func),
                          a);
  CHECK_FALSE(invoked);
  auto handlers = ctx.run_one();
  CHECK(handlers == 1);
  CHECK(invoked);
}

TEST_CASE("execution_context executor_type equality",
          "[execution_context]")
{
  execution_context ctx(100);
  CHECK(ctx.get_executor() == ctx.get_executor());
  CHECK_FALSE(ctx.get_executor() != ctx.get_executor());
  execution_context ctx2(100);
  CHECK(ctx.get_executor() != ctx2.get_executor());
  CHECK_FALSE(ctx.get_executor() == ctx2.get_executor());
}

TEST_CASE("execution_context executor_type boost::asio::post",
          "[execution_context][asio]")
{
  execution_context ctx(100);
  bool invoked = false;
  boost::asio::post(ctx.get_executor(),
                    [&]() { invoked = true; });
  CHECK_FALSE(invoked);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  CHECK(invoked);
}

TEST_CASE("execution_context poll",
          "[execution_context]")
{
  execution_context ctx(100);
  std::size_t i(0);
  std::optional<std::size_t> a;
  std::optional<std::size_t> b;
  std::allocator<void> alloc;
  auto handlers = ctx.poll();
  CHECK(handlers == 0);
  ctx.get_executor().dispatch([&]() { a = i++; },
                              alloc);
  ctx.get_executor().dispatch([&]() { b = i++; },
                              alloc);
  handlers = ctx.poll();
  CHECK(handlers == 0);
  CHECK_FALSE(a);
  CHECK_FALSE(b);
  ctx.restart();
  handlers = ctx.poll();
  CHECK(handlers == 2);
  REQUIRE(a);
  CHECK(*a == 0);
  REQUIRE(b);
  CHECK(*b == 1);
  i = 0;
  a = std::nullopt;
  b = std::nullopt;
  ctx.get_executor().dispatch([&]() { a = i++; },
                              alloc);
  ctx.get_executor().dispatch([&]() { b = i++; },
                              alloc);
  handlers = ctx.poll();
  CHECK(handlers == 0);
  ctx.restart();
  handlers = ctx.poll();
  CHECK(handlers == 2);
  REQUIRE(a);
  CHECK(*a == 0);
  REQUIRE(b);
  CHECK(*b == 1);
  ctx.restart();
  handlers = ctx.poll();
  CHECK(handlers == 0);
}

TEST_CASE("execution_context poll_one",
          "[execution_context]")
{
  execution_context ctx(100);
  std::size_t i(0);
  std::optional<std::size_t> a;
  std::optional<std::size_t> b;
  std::allocator<void> alloc;
  auto handlers = ctx.poll_one();
  CHECK(handlers == 0);
  ctx.get_executor().dispatch([&]() { a = i++; },
                              alloc);
  ctx.get_executor().dispatch([&]() { b = i++; },
                              alloc);
  handlers = ctx.poll_one();
  CHECK(handlers == 0);
  CHECK_FALSE(a);
  CHECK_FALSE(b);
  ctx.restart();
  handlers = ctx.poll_one();
  CHECK(handlers == 1);
  REQUIRE(a);
  CHECK(*a == 0);
  CHECK_FALSE(b);
  handlers = ctx.poll_one();
  CHECK(handlers == 0);
  CHECK_FALSE(b);
  ctx.restart();
  handlers = ctx.poll_one();
  CHECK(handlers == 1);
  REQUIRE(b);
  CHECK(*b == 1);
  i = 0;
  a = std::nullopt;
  b = std::nullopt;
  ctx.get_executor().dispatch([&]() { a = i++; },
                              alloc);
  ctx.get_executor().dispatch([&]() { b = i++; },
                              alloc);
  handlers = ctx.poll();
  CHECK(handlers == 0);
  ctx.restart();
  handlers = ctx.poll_one();
  CHECK(handlers == 1);
  REQUIRE(a);
  CHECK(*a == 0);
  CHECK_FALSE(b);
  handlers = ctx.poll_one();
  CHECK(handlers == 0);
  CHECK_FALSE(b);
  ctx.restart();
  handlers = ctx.poll_one();
  CHECK(handlers == 1);
  REQUIRE(b);
  CHECK(*b == 1);
  ctx.restart();
  handlers = ctx.poll_one();
  CHECK(handlers == 0);
}

TEST_CASE("execution_context completion",
          "[execution_context]")
{
  class completion : public execution_context::completion {
  public:
    completion(execution_context& ctx,
               std::optional<::io_uring_cqe>& cqe) noexcept
      : ctx_(ctx),
        cqe_(cqe)
    {}
    virtual void complete(const ::io_uring_cqe& cqe) override {
      REQUIRE_FALSE(cqe_);
      ctx_.get_executor().on_work_finished();
      cqe_ = cqe;
    }
  public:
    execution_context&             ctx_;
    std::optional<::io_uring_cqe>& cqe_;
  };
  execution_context ctx(100);
  std::optional<::io_uring_cqe> cqe;
  completion c(ctx,
               cqe);
  auto sqe = ::io_uring_get_sqe(ctx.native_handle());
  REQUIRE(sqe);
  ::io_uring_prep_nop(sqe);
  ::io_uring_sqe_set_data(sqe,
                          &c);
  int result = ::io_uring_submit(ctx.native_handle());
  REQUIRE(result >= 0);
  auto handlers = ctx.run();
  CHECK(handlers == 0);
  CHECK_FALSE(cqe);
  ctx.restart();
  ctx.get_executor().on_work_started();
  handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(cqe);
  CHECK(cqe->res == 0);
  CHECK(cqe->flags == 0);
}

TEST_CASE("execution_context get_sqe") {
  execution_context ctx(1);
  ctx.get_sqe();
  std::error_code ec;
  REQUIRE_FALSE(ec);
  try {
    ctx.get_sqe();
  } catch (const std::system_error& ex) {
    ec = ex.code();
  }
  CHECK(ec);
  CHECK(ec.default_error_condition() == std::error_code(EBUSY,
                                                        std::generic_category()).default_error_condition());
}

}
}
