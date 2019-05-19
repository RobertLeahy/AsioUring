#include <asio_uring/asio/fd_completion_token.hpp>

#include <chrono>
#include <future>
#include <memory>
#include <optional>
#include <utility>
#include <asio_uring/asio/execution_context.hpp>
#include <asio_uring/fd.hpp>
#include <asio_uring/test/allocator.hpp>
#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/use_future.hpp>
#include <errno.h>
#include <unistd.h>

#include <catch2/catch.hpp>

namespace asio_uring::asio::tests {
namespace {

TEST_CASE("fd_completion_token basic",
          "[fd_completion_token]")
{
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  auto ptr = std::make_shared<fd>(std::move(write));
  std::optional<int> inner_result;
  char c = 'A';
  auto func = [&]() {
    inner_result = ::write(pipes[1],
                           &c,
                           sizeof(c));
  };
  fd_completion_token token(func,
                            ptr);
  CHECK(ptr.use_count() == 2);
  using async_result_type = boost::asio::async_result<decltype(token),
                                                      void()>;
  async_result_type::completion_handler_type h(std::move(token));
  CHECK(ptr.use_count() == 2);
  async_result_type r(h);
  CHECK(ptr.use_count() == 2);
  r.get();
  CHECK(ptr.use_count() == 2);
  ptr.reset();
  CHECK_FALSE(inner_result);
  h();
  REQUIRE(inner_result);
  CHECK(*inner_result == 1);
  result = ::write(pipes[1],
                   &c,
                   sizeof(c));
  REQUIRE(result == -1);
  CHECK(errno == EBADF);
}

TEST_CASE("fd_completion_token customized return type",
          "[fd_completion_token]")
{
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  auto ptr = std::make_shared<fd>(std::move(write));
  fd_completion_token token(boost::asio::use_future,
                            ptr);
  CHECK(ptr.use_count() == 2);
  using async_result_type = boost::asio::async_result<decltype(token),
                                                      void(int)>;
  async_result_type::completion_handler_type h(std::move(token));
  CHECK(ptr.use_count() == 2);
  async_result_type r(h);
  CHECK(ptr.use_count() == 2);
  auto future = r.get();
  CHECK(ptr.use_count() == 2);
  auto status = future.wait_for(std::chrono::seconds(0));
  CHECK(status == std::future_status::timeout);
  h(4);
  CHECK(ptr.use_count() == 1);
  status = future.wait_for(std::chrono::seconds(0));
  REQUIRE(status == std::future_status::ready);
  auto i = future.get();
  CHECK(i == 4);
}

class has_associated_executor {
public:
  using executor_type = execution_context::executor_type;
  explicit has_associated_executor(const executor_type& ex) noexcept
    : ex_(ex)
  {}
  executor_type get_executor() const noexcept {
    return ex_;
  }
  void operator()() {}
private:
  executor_type ex_;
};

TEST_CASE("fd_completion_token associated executor",
          "[fd_completion_token]")
{
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  auto ptr = std::make_shared<fd>(std::move(write));
  execution_context a(10);
  execution_context b(10);
  has_associated_executor h(a.get_executor());
  CHECK(a.get_executor() == boost::asio::get_associated_executor(h,
                                                                 b.get_executor()));
  CHECK(b.get_executor() != boost::asio::get_associated_executor(h,
                                                                 b.get_executor()));
  fd_completion_token token(h,
                            ptr);
  using async_result_type = boost::asio::async_result<decltype(token),
                                                      void()>;
  async_result_type::completion_handler_type wrapped(std::move(token));
  CHECK(a.get_executor() == boost::asio::get_associated_executor(wrapped,
                                                                 b.get_executor()));
  CHECK(b.get_executor() != boost::asio::get_associated_executor(wrapped,
                                                                 b.get_executor()));
}

class has_associated_allocator {
public:
  using allocator_type = test::allocator<void>;
  explicit has_associated_allocator(const allocator_type& alloc) noexcept
    : alloc_(alloc)
  {}
  allocator_type get_allocator() const noexcept {
    return alloc_;
  }
  void operator()() {}
private:
  allocator_type alloc_;
};

TEST_CASE("fd_completion_token associated allocator",
          "[fd_completion_token]")
{
  using allocator_type = test::allocator<void>;
  allocator_type::state_type s1;
  allocator_type a(s1);
  allocator_type::state_type s2;
  allocator_type b(s2);
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  auto ptr = std::make_shared<fd>(std::move(write));
  has_associated_allocator h(a);
  CHECK(a == boost::asio::get_associated_allocator(h,
                                                   b));
  CHECK(b != boost::asio::get_associated_allocator(h,
                                                   b));
  fd_completion_token token(h,
                            ptr);
  using async_result_type = boost::asio::async_result<decltype(token),
                                                      void()>;
  async_result_type::completion_handler_type wrapped(std::move(token));
  CHECK(a == boost::asio::get_associated_allocator(wrapped,
                                                   b));
  CHECK(b != boost::asio::get_associated_allocator(wrapped,
                                                   b));
}

}
}
