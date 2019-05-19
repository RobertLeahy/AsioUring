#include <asio_uring/asio/file_object.hpp>

#include <optional>
#include <utility>
#include <asio_uring/asio/execution_context.hpp>
#include <asio_uring/fd.hpp>
#include <boost/asio/async_result.hpp>
#include <errno.h>
#include <unistd.h>

#include <catch2/catch.hpp>

namespace asio_uring::asio::tests {
namespace {

TEST_CASE("file_object native_handle",
          "[file_object]")
{
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  execution_context ctx(10);
  file_object obj(ctx,
                  std::move(read));
  CHECK(obj.native_handle() == pipes[0]);
}

class derived : public file_object {
public:
  using file_object::file_object;
  using file_object::wrap_handler;
  using file_object::wrap_token;
  using file_object::reset;
  using file_object::outstanding;
};

TEST_CASE("file_object outstanding & wrap_handler",
          "[file_object]")
{
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  execution_context ctx(10);
  derived obj(ctx,
              std::move(write));
  CHECK(obj.outstanding() == 0);
  bool invoked = false;
  auto wrapped = obj.wrap_handler([&]() noexcept { invoked = true; });
  CHECK(obj.outstanding() == 1);
  auto wrapped2 = obj.wrap_handler([]() noexcept {});
  CHECK(obj.outstanding() == 2);
  CHECK_FALSE(invoked);
  wrapped();
  CHECK(invoked);
  CHECK(obj.outstanding() == 1);
}

TEST_CASE("file_object wrap_handler",
          "[file_object]")
{
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  execution_context ctx(10);
  derived obj(ctx,
              std::move(write));
  char c = 'A';
  result = ::write(obj.native_handle(),
                   &c,
                   sizeof(c));
  CHECK(result == 1);
  std::optional<int> inner_result;
  auto wrapped = obj.wrap_handler([&]() noexcept {
    inner_result = ::write(pipes[1],
                           &c,
                           sizeof(c));
  });
  obj.reset();
  CHECK_FALSE(inner_result);
  wrapped();
  REQUIRE(inner_result);
  CHECK(*inner_result == 1);
  result = ::write(pipes[1],
                   &c,
                   sizeof(c));
  REQUIRE(result == -1);
  CHECK(errno == EBADF);
}

TEST_CASE("file_object reset",
          "[file_object]")
{
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  execution_context ctx(10);
  derived obj(ctx,
              std::move(write));
  char c = 'A';
  result = ::write(obj.native_handle(),
                   &c,
                   sizeof(c));
  CHECK(result == 1);
  obj.reset();
  result = ::write(pipes[1],
                   &c,
                   sizeof(c));
  REQUIRE(result == -1);
  CHECK(errno == EBADF);
}

TEST_CASE("file_object outstanding & wrap_token",
          "[file_object]")
{
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  execution_context ctx(10);
  derived obj(ctx,
              std::move(write));
  CHECK(obj.outstanding() == 0);
  std::optional<long> outstanding;
  auto wrapped = obj.wrap_token([&]() noexcept { outstanding = obj.outstanding(); });
  CHECK(obj.outstanding() == 1);
  CHECK_FALSE(outstanding);
  using async_result_type = boost::asio::async_result<decltype(wrapped),
                                                      void()>;
  async_result_type::completion_handler_type h(std::move(wrapped));
  CHECK(obj.outstanding() == 1);
  async_result_type r(h);
  CHECK(obj.outstanding() == 1);
  r.get();
  CHECK(obj.outstanding() == 1);
  h();
  CHECK(obj.outstanding() == 0);
  REQUIRE(outstanding);
  CHECK(*outstanding == 1);
}

}
}
