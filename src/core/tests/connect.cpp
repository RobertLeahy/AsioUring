#include <asio_uring/connect.hpp>

#include <cstdint>
#include <cstring>
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

#include <catch2/catch.hpp>

namespace asio_uring::tests {
namespace {

TEST_CASE("connect",
          "[connect]")
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
  result = ::fcntl(connect.native_handle(),
                   F_GETFL);
  REQUIRE(result >= 0);
  result |= O_NONBLOCK;
  result = ::fcntl(connect.native_handle(),
                   F_SETFL,
                   result);
  REQUIRE(result == 0);
  auto ec = make_error_code(std::errc::not_enough_memory);
  auto connected = asio_uring::connect(connect.native_handle(),
                                       addr,
                                       ec);
  REQUIRE_FALSE(ec);
  if (!connected) {
    INFO("Connect didn't complete immediately");
    ::pollfd pfd;
    std::memset(&pfd,
                0,
                sizeof(pfd));
    pfd.fd = connect.native_handle();
    pfd.events = POLLOUT;
    result = ::poll(&pfd,
                    1,
                    -1);
    REQUIRE(result == 1);
    int e;
    addr_len = sizeof(e);
    result = ::getsockopt(connect.native_handle(),
                          SOL_SOCKET,
                          SO_ERROR,
                          &e,
                          &addr_len);
    REQUIRE(result == 0);
    CHECK(e == 0);
  }
}

TEST_CASE("connect bad file descriptor",
          "[connect]")
{
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
  std::uint16_t port = 11653;
  boost::endian::native_to_big_inplace(port);
  addr.sin_port = port;
  auto ec = make_error_code(std::errc::not_enough_memory);
  connect(-1,
          addr,
          ec);
  {
    INFO(ec.message());
    CHECK(ec);
    CHECK(ec == std::error_code(EBADF,
                                std::generic_category()));
  }
}

TEST_CASE("connect_error bad file descriptor",
          "[connect_error]")
{
  auto ec = connect_error(-1);
  CHECK(ec);
  CHECK(ec == std::error_code(EBADF,
                              std::generic_category()));
}

}
}
