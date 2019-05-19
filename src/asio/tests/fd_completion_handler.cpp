#include <asio_uring/asio/fd_completion_handler.hpp>

#include <memory>
#include <optional>
#include <utility>
#include <asio_uring/asio/execution_context.hpp>
#include <asio_uring/fd.hpp>
#include <asio_uring/test/allocator.hpp>
#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/associated_executor.hpp>
#include <unistd.h>

#include <catch2/catch.hpp>

namespace asio_uring::asio::tests {
namespace {

TEST_CASE("fd_completion_handler",
          "[fd_completion_handler]")
{
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  auto ptr = std::make_shared<fd>(std::move(read));
  REQUIRE(ptr.use_count() == 1);
  std::optional<long> use_count;
  fd_completion_handler h([&]() noexcept { use_count = ptr.use_count(); },
                          ptr);
  REQUIRE(ptr.use_count() == 2);
  CHECK_FALSE(use_count);
  h();
  CHECK(ptr.use_count() == 1);
  REQUIRE(use_count);
  CHECK(*use_count == 2);
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

TEST_CASE("fd_completion_handler associated allocator",
          "[fd_completion_handler]")
{
  using allocator_type = test::allocator<void>;
  allocator_type::state_type s;
  allocator_type a(s);
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  auto ptr = std::make_shared<fd>(std::move(read));
  has_associated_allocator h(a);
  fd_completion_handler wrapped(h,
                                ptr);
  allocator_type::state_type s2;
  allocator_type a2(s2);
  CHECK(a == boost::asio::get_associated_allocator(h,
                                                   a2));
  CHECK(a2 != boost::asio::get_associated_allocator(h,
                                                    a2));
  CHECK(a == boost::asio::get_associated_allocator(wrapped,
                                                   a2));
  CHECK(a2 != boost::asio::get_associated_allocator(wrapped,
                                                    a2));
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
private:
  executor_type ex_;
};

TEST_CASE("fd_completion_handler associated executor",
          "[fd_completion_handler]")
{
  execution_context a(10);
  execution_context b(10);
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  auto ptr = std::make_shared<fd>(std::move(read));
  has_associated_executor h(a.get_executor());
  fd_completion_handler wrapped(h,
                                ptr);
  CHECK(a.get_executor() == boost::asio::get_associated_executor(h,
                                                                 b.get_executor()));
  CHECK(b.get_executor() != boost::asio::get_associated_executor(h,
                                                                 b.get_executor()));
  CHECK(a.get_executor() == boost::asio::get_associated_executor(wrapped,
                                                                 b.get_executor()));
  CHECK(b.get_executor() != boost::asio::get_associated_executor(wrapped,
                                                                 b.get_executor()));
}

}
}
