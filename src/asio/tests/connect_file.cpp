#include <asio_uring/asio/connect_file.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <system_error>
#include <utility>
#include <asio_uring/asio/execution_context.hpp>
#include <asio_uring/fd.hpp>
#include <boost/endian/conversion.hpp>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <catch2/catch.hpp>

namespace asio_uring::asio::tests {
namespace {

TEST_CASE("connect_file async_connect",
          "[connect_file]")
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
  std::optional<std::error_code> ec;
  execution_context ctx(10);
  connect_file poll(ctx,
                    std::move(connect));
  poll.async_connect(addr,
                     [&](auto e) noexcept { ec = e; });
  CHECK_FALSE(ec);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK_FALSE(*ec);
}

TEST_CASE("connect_file async_connect bad destination",
          "[connect_file]")
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
  std::uint16_t port = 11655;
  boost::endian::native_to_big_inplace(port);
  addr.sin_port = port;
  fd connect(::socket(AF_INET,
                      SOCK_STREAM,
                      0));
  auto result = ::fcntl(connect.native_handle(),
                        F_GETFL);
  REQUIRE(result >= 0);
  result |= O_NONBLOCK;
  result = ::fcntl(connect.native_handle(),
                   F_SETFL,
                   result);
  REQUIRE(result == 0);
  std::optional<std::error_code> ec;
  execution_context ctx(10);
  connect_file poll(ctx,
                    std::move(connect));
  poll.async_connect(addr,
                     [&](auto e) noexcept { ec = e; });
  CHECK_FALSE(ec);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK(*ec);
}

}
}
