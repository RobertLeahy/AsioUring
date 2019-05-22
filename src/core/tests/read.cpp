#include <asio_uring/read.hpp>

#include <algorithm>
#include <cstring>
#include <iterator>
#include <string_view>
#include <system_error>
#include <asio_uring/fd.hpp>
#include <boost/endian/conversion.hpp>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
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

TEST_CASE("read eof",
          "[read]")
{
  fd listen(::socket(AF_INET,
                     SOCK_STREAM,
                     0));
  ::sockaddr_in addr;
  std::memset(&addr,
              0,
              sizeof(addr));
  addr.sin_family = AF_INET;
  std::uint32_t ip = 127;
  ip <<= 8;
  ip <<= 8;
  ip <<= 8;
  ip |= 1;
  boost::endian::native_to_big_inplace(ip);
  addr.sin_addr.s_addr = ip;
  auto result = ::bind(listen.native_handle(),
                       reinterpret_cast<const ::sockaddr*>(&addr),
                       sizeof(addr));
  REQUIRE(result == 0);
  ::socklen_t addr_len = sizeof(addr);
  result = ::getsockname(listen.native_handle(),
                         reinterpret_cast<::sockaddr*>(&addr),
                         &addr_len);
  REQUIRE(result == 0);
  REQUIRE(addr.sin_port != 0);
  result = ::listen(listen.native_handle(),
                    1);
  REQUIRE(result == 0);
  fd connect(::socket(AF_INET,
                      SOCK_STREAM,
                      0));
  result = ::connect(connect.native_handle(),
                     reinterpret_cast<const ::sockaddr*>(&addr),
                     sizeof(addr));
  REQUIRE(result == 0);
  fd accepted(::accept4(listen.native_handle(),
                        nullptr,
                        nullptr,
                        SOCK_NONBLOCK));
  listen = fd();
  ::pollfd pfd;
  std::memset(&pfd,
              0,
              sizeof(pfd));
  pfd.fd = accepted.native_handle();
  pfd.events = POLLIN;
  result = ::poll(&pfd,
                  1,
                  0);
  REQUIRE(result == 0);
  result = ::shutdown(connect.native_handle(),
                      SHUT_RDWR);
  REQUIRE(result == 0);
  result = ::poll(&pfd,
                  1,
                  0);
  REQUIRE(result > 0);
  std::error_code ec;
  char c;
  auto bytes = read(accepted.native_handle(),
                    &c,
                    sizeof(c),
                    ec);
  REQUIRE_FALSE(ec);
  CHECK(bytes == 0);
}

}
}
