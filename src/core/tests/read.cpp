#include <asio_uring/read.hpp>

#include <algorithm>
#include <iterator>
#include <string_view>
#include <system_error>
#include <asio_uring/fd.hpp>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <catch2/catch.hpp>

namespace asio_uring::tests {
namespace {

TEST_CASE("read empty",
          "[read]")
{
  int pipes[2];
  auto result = ::pipe2(pipes,
                        O_NONBLOCK);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  std::error_code ec = make_error_code(std::errc::not_enough_memory);
  auto bytes = asio_uring::read(read.native_handle(),
                                nullptr,
                                0,
                                ec);
  CHECK_FALSE(ec);
  CHECK(bytes == 0);
}

TEST_CASE("read",
          "[read]")
{
  int pipes[2];
  auto result = ::pipe2(pipes,
                        O_NONBLOCK);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  std::string_view sv("Hello world!");
  auto written = ::write(write.native_handle(),
                         sv.data(),
                         sv.size());
  REQUIRE(written == sv.size());
  char buffer[5];
  std::error_code ec = make_error_code(std::errc::not_enough_memory);
  auto bytes = asio_uring::read(read.native_handle(),
                                buffer,
                                sizeof(buffer),
                                ec);
  REQUIRE_FALSE(ec);
  REQUIRE(bytes == sizeof(buffer));
  CHECK(std::equal(sv.begin(),
                   sv.begin() + 5,
                   std::begin(buffer),
                   std::end(buffer)));
  bytes = asio_uring::read(read.native_handle(),
                           buffer,
                           sizeof(buffer),
                           ec);
  REQUIRE_FALSE(ec);
  REQUIRE(bytes == sizeof(buffer));
  CHECK(std::equal(sv.begin() + 5,
                   sv.begin() + 10,
                   std::begin(buffer),
                   std::end(buffer)));
  bytes = asio_uring::read(read.native_handle(),
                           buffer,
                           sizeof(buffer),
                           ec);
  {
    INFO(ec.message());
    REQUIRE_FALSE(ec);
  }
  REQUIRE(bytes == 2);
  CHECK(std::equal(sv.begin() + 10,
                   sv.end(),
                   std::begin(buffer),
                   std::begin(buffer) + 2));
}

TEST_CASE("read bad file",
          "[read]")
{
  char buffer[5];
  std::error_code ec;
  auto bytes = asio_uring::read(-1,
                                buffer,
                                sizeof(buffer),
                                ec);
  CHECK(ec);
  CHECK(bytes == 0);
  CHECK(ec == std::error_code(EBADF,
                              std::generic_category()));
}

}
}
