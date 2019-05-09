#include <asio_uring/fd.hpp>

#include <optional>
#include <system_error>
#include <utility>
#include <errno.h>
#include <unistd.h>

#include <catch2/catch.hpp>

namespace asio_uring::tests {
namespace {

TEST_CASE("fd bad handle",
          "[fd]")
{
  std::optional<std::error_code> ec;
  errno = EAGAIN;
  try {
    fd file(-1);
  } catch (const std::system_error& ex) {
    ec = ex.code();
  }
  REQUIRE(ec);
  CHECK(ec->value() == EAGAIN);
  CHECK(&ec->category() == &std::generic_category());
}

TEST_CASE("fd default construct",
          "[fd]")
{
  fd file;
  char c = 'A';
  ::ssize_t bytes = ::write(file.native_handle(),
                            &c,
                            1);
  CHECK(bytes == -1);
}

TEST_CASE("fd close",
          "[fd]")
{
  int pipes[2];
  int result = ::pipe(pipes);
  REQUIRE_FALSE(result);
  std::optional<fd> read(std::in_place,
                         pipes[0]);
  std::optional<fd> write(std::in_place,
                          pipes[1]);
  write = std::nullopt;
  char c;
  ::ssize_t bytes = ::read(read->native_handle(),
                           &c,
                           1);
  CHECK(bytes == 0);
}

TEST_CASE("fd move construct",
          "[fd]")
{
  int pipes[2];
  int result = ::pipe(pipes);
  REQUIRE_FALSE(result);
  fd read(pipes[0]);
  fd write(pipes[1]);
  fd moved_write(std::move(write));
  char c = 'A';
  ::ssize_t bytes = ::write(moved_write.native_handle(),
                            &c,
                            1);
  REQUIRE(bytes == 1);
  bytes = ::read(read.native_handle(),
                 &c,
                 1);
  REQUIRE(bytes == 1);
  CHECK(c == 'A');
}

TEST_CASE("fd move assign",
          "[fd]")
{
  int pipes[2];
  int result = ::pipe(pipes);
  REQUIRE_FALSE(result);
  fd read_a(pipes[0]);
  fd write_a(pipes[1]);
  result = ::pipe(pipes);
  REQUIRE_FALSE(result);
  fd read_b(pipes[0]);
  fd write_b(pipes[1]);
  write_b = std::move(write_a);
  char c;
  ::ssize_t bytes = ::read(read_b.native_handle(),
                           &c,
                           1);
  CHECK(bytes == 0);
  c = 'A';
  bytes = ::write(write_b.native_handle(),
                  &c,
                  1);
  REQUIRE(bytes == 1);
  bytes = ::read(read_a.native_handle(),
                 &c,
                 1);
  REQUIRE(bytes == 1);
  CHECK(c == 'A');
}

}
}
