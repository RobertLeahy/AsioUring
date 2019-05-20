#include <asio_uring/accept.hpp>

#include <cstdint>
#include <cstring>
#include <system_error>
#include <asio_uring/fd.hpp>
#include <boost/endian/conversion.hpp>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <catch2/catch.hpp>

namespace asio_uring::tests {
namespace {

TEST_CASE("accept",
          "[accept]")
{
  fd listen(::socket(AF_INET,
                     SOCK_STREAM,
                     0));
  auto result = ::fcntl(listen.native_handle(),
                        F_GETFL);
  REQUIRE(result >= 0);
  result = ::fcntl(listen.native_handle(),
                   F_SETFL,
                   result | O_NONBLOCK);
  REQUIRE(result == 0);
  result = ::fcntl(listen.native_handle(),
                   F_GETFL);
  REQUIRE(result >= 0);
  REQUIRE((result & O_NONBLOCK) != 0);
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
  result = ::bind(listen.native_handle(),
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
  ::sockaddr_in accepted_address;
  auto ec = make_error_code(std::errc::not_enough_memory);
  auto accepted = accept(listen.native_handle(),
                         accepted_address,
                         ec);
  REQUIRE_FALSE(ec);
  REQUIRE(accepted);
  CHECK(accepted_address.sin_family == addr.sin_family);
  CHECK(accepted_address.sin_addr.s_addr == addr.sin_addr.s_addr);
  result = ::fcntl(accepted->native_handle(),
                   F_GETFL);
  REQUIRE(result >= 0);
  REQUIRE((result & O_NONBLOCK) != 0);
}

TEST_CASE("accept binary",
          "[accept]")
{
  fd listen(::socket(AF_INET,
                     SOCK_STREAM,
                     0));
  auto result = ::fcntl(listen.native_handle(),
                        F_GETFL);
  REQUIRE(result >= 0);
  result = ::fcntl(listen.native_handle(),
                   F_SETFL,
                   result | O_NONBLOCK);
  REQUIRE(result == 0);
  result = ::fcntl(listen.native_handle(),
                   F_GETFL);
  REQUIRE(result >= 0);
  REQUIRE((result & O_NONBLOCK) != 0);
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
  result = ::bind(listen.native_handle(),
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
  auto ec = make_error_code(std::errc::not_enough_memory);
  auto accepted = accept(listen.native_handle(),
                         ec);
  REQUIRE_FALSE(ec);
  REQUIRE(accepted);
  result = ::fcntl(accepted->native_handle(),
                   F_GETFL);
  REQUIRE(result >= 0);
  REQUIRE((result & O_NONBLOCK) != 0);
}

TEST_CASE("accept bad file descriptor",
          "[accept]")
{
  ::sockaddr_in addr;
  std::error_code ec;
  accept(-1,
         addr,
         ec);
  CHECK(ec);
  CHECK(ec == std::error_code(EBADF,
                              std::generic_category()));
}

TEST_CASE("accept would block",
          "[accept]")
{
  fd listen(::socket(AF_INET,
                     SOCK_STREAM,
                     0));
  auto result = ::fcntl(listen.native_handle(),
                        F_GETFL);
  REQUIRE(result >= 0);
  result = ::fcntl(listen.native_handle(),
                   F_SETFL,
                   result | O_NONBLOCK);
  REQUIRE(result == 0);
  result = ::fcntl(listen.native_handle(),
                   F_GETFL);
  REQUIRE(result >= 0);
  REQUIRE((result & O_NONBLOCK) != 0);
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
  result = ::bind(listen.native_handle(),
                  reinterpret_cast<const ::sockaddr*>(&addr),
                  sizeof(addr));
  REQUIRE(result == 0);
  result = ::listen(listen.native_handle(),
                    1);
  REQUIRE(result == 0);
  auto ec = make_error_code(std::errc::not_enough_memory);
  auto accepted = accept(listen.native_handle(),
                         addr,
                         ec);
  CHECK_FALSE(ec);
  CHECK_FALSE(accepted);
}

}
}
