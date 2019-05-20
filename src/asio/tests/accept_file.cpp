#include <asio_uring/asio/accept_file.hpp>

#include <cstdint>
#include <cstring>
#include <optional>
#include <system_error>
#include <utility>
#include <asio_uring/asio/execution_context.hpp>
#include <asio_uring/fd.hpp>
#include <boost/endian/conversion.hpp>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <catch2/catch.hpp>

namespace asio_uring::asio::tests {
namespace {

TEST_CASE("accept_file unary async_accept",
          "[accept_file]")
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
  using pair_type = std::pair<std::error_code,
                              fd>;
  std::optional<pair_type> pair;
  execution_context ctx(10);
  accept_file accept(ctx,
                     std::move(listen));
  accept.async_accept([&](auto ec,
                          auto fd) noexcept
                      {
                        pair.emplace(ec,
                                     std::move(fd));
                      });
  CHECK_FALSE(pair);
  auto handlers = ctx.poll();
  CHECK(handlers == 0);
  ctx.restart();
  fd connect(::socket(AF_INET,
                      SOCK_STREAM,
                      0));
  result = ::connect(connect.native_handle(),
                     reinterpret_cast<const ::sockaddr*>(&addr),
                     sizeof(addr));
  REQUIRE(result == 0);
  handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(pair);
  REQUIRE_FALSE(pair->first);
  result = ::fcntl(pair->second.native_handle(),
                   F_GETFL);
  REQUIRE(result >= 0);
  CHECK((result & O_NONBLOCK) != 0);
}

TEST_CASE("accept_file binary async_accept",
          "[accept_file]")
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
  using pair_type = std::pair<std::error_code,
                              fd>;
  std::optional<pair_type> pair;
  ::sockaddr_in accepted_addr;
  execution_context ctx(10);
  accept_file accept(ctx,
                     std::move(listen));
  accept.async_accept(accepted_addr,
                      [&](auto ec,
                          auto fd) noexcept
                      {
                        pair.emplace(ec,
                                     std::move(fd));
                      });
  CHECK_FALSE(pair);
  auto handlers = ctx.poll();
  CHECK(handlers == 0);
  ctx.restart();
  fd connect(::socket(AF_INET,
                      SOCK_STREAM,
                      0));
  result = ::connect(connect.native_handle(),
                     reinterpret_cast<const ::sockaddr*>(&addr),
                     sizeof(addr));
  REQUIRE(result == 0);
  handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(pair);
  REQUIRE_FALSE(pair->first);
  CHECK(accepted_addr.sin_family == addr.sin_family);
  CHECK(accepted_addr.sin_addr.s_addr == addr.sin_addr.s_addr);
  CHECK(accepted_addr.sin_port != 0);
  result = ::fcntl(pair->second.native_handle(),
                   F_GETFL);
  REQUIRE(result >= 0);
  CHECK((result & O_NONBLOCK) != 0);
}

}
}
