#include <asio_uring/write.hpp>

#include <system_error>
#include <vector>
#include <asio_uring/fd.hpp>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <catch2/catch.hpp>

namespace asio_uring::tests {
namespace {

TEST_CASE("write empty",
          "[write]")
{
  int pipes[2];
  auto result = ::pipe2(pipes,
                        O_NONBLOCK);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  std::error_code ec = make_error_code(std::errc::not_enough_memory);
  auto written = asio_uring::write(write.native_handle(),
                                   nullptr,
                                   0,
                                   ec);
  CHECK_FALSE(ec);
  CHECK(written == 0);
}

TEST_CASE("write would block",
          "[write]")
{
  int pipes[2];
  auto result = ::pipe2(pipes,
                        O_NONBLOCK);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  auto size = ::fcntl(write.native_handle(),
                      F_GETPIPE_SZ);
  REQUIRE(size > 0);
  std::vector<char> vec;
  vec.resize(size,
             'A');
  auto written = ::write(write.native_handle(),
                         vec.data(),
                         vec.size());
  REQUIRE(written == vec.size());
  std::error_code ec;
  char c = 'A';
  auto bytes = asio_uring::write(write.native_handle(),
                                 &c,
                                 sizeof(c),
                                 ec);
  CHECK_FALSE(ec);
  CHECK(bytes == 0);
}

TEST_CASE("write",
          "[write]")
{
  int pipes[2];
  auto result = ::pipe2(pipes,
                        O_NONBLOCK);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  char c = 'A';
  std::error_code ec;
  auto written = asio_uring::write(write.native_handle(),
                                   &c,
                                   sizeof(c),
                                   ec);
  REQUIRE_FALSE(ec);
  CHECK(written == 1);
}

TEST_CASE("write bad fd",
          "[write]")
{
  char c = 'A';
  std::error_code ec;
  auto written = asio_uring::write(-1,
                                   &c,
                                   sizeof(c),
                                   ec);
  CHECK(ec);
  CHECK(written == 0);
  CHECK(ec == std::error_code(EBADF,
                              std::generic_category()));
}

}
}
