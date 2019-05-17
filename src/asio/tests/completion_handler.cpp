#include <asio_uring/asio/completion_handler.hpp>

#include <atomic>
#include <memory>
#include <thread>
#include <asio_uring/asio/execution_context.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/core/noncopyable.hpp>

#include <catch2/catch.hpp>

namespace asio_uring::asio::tests {
namespace {

TEST_CASE("completion_handler nullary no customization",
          "[completion_handler]")
{
  execution_context ctx(10);
  bool invoked = false;
  completion_handler h([&]() { invoked = true; },
                       std::allocator<void>(),
                       ctx.get_executor());
  CHECK_FALSE(invoked);
  auto handlers = ctx.poll();
  CHECK(handlers == 0);
  CHECK_FALSE(invoked);
  h();
  CHECK_FALSE(invoked);
  ctx.restart();
  handlers = ctx.run();
  CHECK(handlers == 1);
  CHECK(invoked);
}

TEST_CASE("completion_handler custom executor",
          "[completion_handler]")
{
  boost::asio::io_context ioc;
  execution_context ctx(10);
  bool invoked = false;
  auto bound = boost::asio::bind_executor(ioc.get_executor(),
                                          [&]() { invoked = true; });
  completion_handler h(bound,
                       std::allocator<void>(),
                       ctx.get_executor());
  CHECK_FALSE(invoked);
  auto handlers = ctx.run();
  CHECK(handlers == 0);
  CHECK_FALSE(invoked);
  handlers = ioc.poll();
  CHECK(handlers == 0);
  CHECK_FALSE(invoked);
  h();
  CHECK_FALSE(invoked);
  ctx.restart();
  handlers = ctx.run();
  CHECK(handlers == 0);
  CHECK_FALSE(invoked);
  ioc.restart();
  handlers = ioc.run();
  CHECK(handlers == 1);
  CHECK(invoked);
}

TEST_CASE("completion_handler dispatch",
          "[completion_handler]")
{
  execution_context ctx(10);
  std::size_t i = 0;
  std::size_t inner = 0;
  std::size_t outer = 0;
  completion_handler h([&]() { inner = ++i; },
                       std::allocator<void>(),
                       ctx.get_executor());
  CHECK(i == 0);
  auto handlers = ctx.poll();
  CHECK(handlers == 0);
  CHECK(i == 0);
  auto func = [&]() {
    outer = ++i;
    h();
  };
  ctx.get_executor().dispatch(func,
                              std::allocator<void>());
  CHECK(i == 0);
  ctx.restart();
  handlers = ctx.run();
  CHECK(handlers == 1);
  CHECK(i == 2);
  CHECK(inner == 2);
  CHECK(outer == 1);
}

TEST_CASE("completion_handler custom executor & work",
          "[completion_handler]")
{
  std::atomic<bool> running(false);
  std::atomic<boost::asio::io_context::count_type> t_handlers(0);
  bool invoked = false;
  boost::asio::io_context ioc;
  ioc.get_executor().dispatch([&]() { running = true; },
                              std::allocator<void>());
  execution_context ctx(10);
  auto bound = boost::asio::bind_executor(ioc.get_executor(),
                                          [&]() { invoked = true; });
  completion_handler h(bound,
                       std::allocator<void>(),
                       ctx.get_executor());
  CHECK_FALSE(invoked);
  {
    std::thread t([&]() {
      t_handlers = ioc.run();
    });
    class guard : private boost::noncopyable {
    public:
      explicit guard(std::thread& t) noexcept
        : t_(t)
      {}
      ~guard() noexcept {
        t_.join();
      }
    private:
      std::thread& t_;
    };
    guard g(t);
    while (!running);
    h();
  }
  CHECK(invoked);
  CHECK(t_handlers == 2);
}

TEST_CASE("completion_handler binary constructor & CTAD",
          "[completion_handler]")
{
  execution_context ctx(10);
  bool invoked = false;
  completion_handler h([&]() { invoked = true; },
                       ctx.get_executor());
  CHECK_FALSE(invoked);
  auto handlers = ctx.poll();
  CHECK(handlers == 0);
  CHECK_FALSE(invoked);
  h();
  CHECK_FALSE(invoked);
  ctx.restart();
  CHECK_FALSE(invoked);
  handlers = ctx.run();
  CHECK(handlers == 1);
  CHECK(invoked);
}

}
}
