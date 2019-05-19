#include <asio_uring/asio/write.hpp>

#include <algorithm>
#include <numeric>
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

TEST_CASE("write empty",
          "[write]")
{
  int pipes[2];
  auto result = ::pipe2(pipes,
                        O_NONBLOCK);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  std::vector<boost::asio::const_buffer> bs;
  auto ec = make_error_code(std::errc::not_enough_memory);
  auto written = asio::write(write.native_handle(),
                             bs,
                             ec);
  CHECK_FALSE(ec);
  CHECK(written == 0);
  bs.emplace_back();
  written = asio::write(write.native_handle(),
                        bs,
                        ec);
  CHECK_FALSE(ec);
  CHECK(written == 0);
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
  std::string_view a("Hello");
  std::string_view b(" ");
  std::string_view c("world!");
  std::vector<boost::asio::const_buffer> bs;
  bs.push_back(boost::asio::buffer(a));
  bs.push_back(boost::asio::buffer(b));
  bs.push_back(boost::asio::buffer(c));
  auto ec = make_error_code(std::errc::not_enough_memory);
  auto written = asio::write(write.native_handle(),
                             bs,
                             ec);
  REQUIRE_FALSE(ec);
  REQUIRE(written == 12);
  char buffer[16];
  auto bytes = ::read(read.native_handle(),
                      buffer,
                      sizeof(buffer));
  REQUIRE(bytes == 12);
  CHECK(std::string_view(buffer,
                         12) == "Hello world!");
}

TEST_CASE("write bad file descriptor",
          "[write]")
{
  std::string_view a("Hello");
  std::vector<boost::asio::const_buffer> bs;
  bs.push_back(boost::asio::buffer(a));
  std::error_code ec;
  auto written = asio::write(-1,
                             bs,
                             ec);
  CHECK(ec);
  CHECK(ec == std::error_code(EBADF,
                              std::generic_category()));
  CHECK(written == 0);
}

TEST_CASE("write incomplete",
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
  std::vector<unsigned char> vec;
  vec.resize(size); 
  unsigned char start(0);
  std::iota(vec.begin(),
            vec.end(),
            start);
  vec.push_back(0);
  std::vector<boost::asio::const_buffer> bs;
  bs.push_back(boost::asio::buffer(vec));
  std::error_code ec;
  auto written = asio::write(write.native_handle(),
                             bs,
                             ec);
  REQUIRE_FALSE(ec);
  CHECK(written == size);
  CHECK(written != vec.size());
  std::vector<unsigned char> vec2;
  vec2.resize(vec.size());
  auto bytes = ::read(read.native_handle(),
                      vec2.data(),
                      vec2.size());
  REQUIRE(bytes == size);
  CHECK(bytes != vec.size());
  CHECK(bytes != vec2.size());
  vec2.pop_back();
  vec.pop_back();
  CHECK(std::equal(vec.begin(),
                   vec.end(),
                   vec2.begin(),
                   vec2.end()));
}

}
}
