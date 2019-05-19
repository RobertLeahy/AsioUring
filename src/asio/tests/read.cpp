#include <asio_uring/asio/read.hpp>

#include <algorithm>
#include <iterator>
#include <string_view>
#include <system_error>
#include <vector>
#include <asio_uring/fd.hpp>
#include <boost/asio/buffer.hpp>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <catch2/catch.hpp>

namespace asio_uring::asio::tests {
namespace {

TEST_CASE("read empty buffer sequence",
          "[read]")
{
  int pipes[2];
  auto result = ::pipe2(pipes,
                        O_NONBLOCK);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  std::vector<boost::asio::mutable_buffer> bs;
  std::error_code ec = make_error_code(std::errc::not_enough_memory);
  auto bytes = asio::read(read.native_handle(),
                          bs,
                          ec);
  CHECK_FALSE(ec);
  CHECK(bytes == 0);
}

TEST_CASE("read non-empty buffer sequence",
          "[read]")
{
  int pipes[2];
  auto result = ::pipe2(pipes,
                        O_NONBLOCK);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  std::string_view sv("1234567890"
                      "1234567890");
  auto written = ::write(write.native_handle(),
                         sv.data(),
                         sv.size());
  REQUIRE(written == sv.size());
  char a[16];
  char b[16];
  std::vector<boost::asio::mutable_buffer> bs;
  bs.push_back(boost::asio::buffer(a));
  bs.push_back(boost::asio::buffer(b));
  std::error_code ec = make_error_code(std::errc::not_enough_memory);
  auto bytes = asio::read(read.native_handle(),
                          bs,
                          ec);
  REQUIRE_FALSE(ec);
  REQUIRE(bytes == sv.size());
  CHECK(std::equal(sv.begin(),
                   sv.begin() + sizeof(a),
                   std::begin(a),
                   std::end(a)));
  CHECK(std::equal(sv.begin() + sizeof(a),
                   sv.end(),
                   std::begin(b),
                   std::begin(b) + 4));
}

TEST_CASE("read bad file descriptor",
          "[read]")
{
  char a[16];
  char b[16];
  std::vector<boost::asio::mutable_buffer> bs;
  bs.push_back(boost::asio::buffer(a));
  bs.push_back(boost::asio::buffer(b));
  std::error_code ec;
  auto bytes = asio::read(-1,
                          bs,
                          ec);
  CHECK(ec);
  CHECK(bytes == 0);
  CHECK(ec == std::error_code(EBADF,
                              std::generic_category()));
}

}
}
