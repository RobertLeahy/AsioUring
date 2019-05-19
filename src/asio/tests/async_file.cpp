#include <asio_uring/asio/async_file.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>
#include <asio_uring/asio/execution_context.hpp>
#include <asio_uring/fd.hpp>
#include <boost/asio/buffer.hpp>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <catch2/catch.hpp>

namespace asio_uring::asio::tests {
namespace {

TEST_CASE("async_file async_read_some_at",
          "[async_file]")
{
  std::string str("Hello world!");
  char filename[] = "/tmp/XXXXXX";
  fd file(::mkstemp(filename));
  INFO("Temporary file is " << filename);
  auto written = ::write(file.native_handle(),
                         str.data(),
                         str.size());
  REQUIRE(written == str.size());
  file = fd();
  file = fd(::open(filename,
                   O_RDONLY));
  char buffer[16];
  using pair_type = std::pair<std::error_code,
                              std::size_t>;
  std::optional<pair_type> a;
  std::optional<pair_type> b;
  std::optional<pair_type> c;
  execution_context ctx(10);
  async_file async(ctx,
                   std::move(file));
  async.async_read_some_at(12,
                           boost::asio::mutable_buffer(buffer + 12,
                                                       sizeof(buffer) - 12),
                           [&](auto ec,
                               auto bytes_transferred) noexcept
                           {
                             a.emplace(ec,
                                       bytes_transferred);
                           });
  async.async_read_some_at(0,
                           boost::asio::mutable_buffer(buffer,
                                                       6),
                           [&](auto ec,
                               auto bytes_transferred) noexcept
                           {
                             b.emplace(ec,
                                       bytes_transferred);
                           });
  async.async_read_some_at(6,
                           boost::asio::mutable_buffer(buffer + 6,
                                                       6),
                           [&](auto ec,
                               auto bytes_transferred) noexcept
                           {
                             c.emplace(ec,
                                       bytes_transferred);
                           });
  CHECK_FALSE(a);
  CHECK_FALSE(b);
  auto handlers = ctx.run();
  CHECK(handlers == 3);
  REQUIRE(a);
  CHECK_FALSE(a->first);
  CHECK(a->second == 0);
  REQUIRE(b);
  REQUIRE_FALSE(b->first);
  REQUIRE(b->second == 6);
  REQUIRE(c);
  REQUIRE_FALSE(c->first);
  REQUIRE(c->second == 6);
  std::string_view sv(buffer,
                      12);
  CHECK(sv == str);
}

TEST_CASE("async_file async_write_some_at",
          "[async_file]")
{
  std::string str("Hello world!");
  char filename[] = "/tmp/XXXXXX";
  fd file(::mkstemp(filename));
  INFO("Temporary file is " << filename);
  using pair_type = std::pair<std::error_code,
                              std::size_t>;
  std::optional<pair_type> a;
  std::optional<pair_type> b;
  std::optional<pair_type> c;
  execution_context ctx(10);
  {
    async_file async(ctx,
                     std::move(file));
    async.async_write_some_at(0,
                              boost::asio::const_buffer(str.data(),
                                                        5),
                              [&](auto ec,
                                  auto bytes_transferred) noexcept
                              {
                                a.emplace(ec,
                                          bytes_transferred);
                              });
    async.async_write_some_at(5,
                              boost::asio::const_buffer(str.data() + 5,
                                                        5),
                              [&](auto ec,
                                  auto bytes_transferred) noexcept
                              {
                                b.emplace(ec,
                                          bytes_transferred);
                              });
    async.async_write_some_at(10,
                              boost::asio::const_buffer(str.data() + 10,
                                                        2),
                              [&](auto ec,
                                  auto bytes_transferred) noexcept
                              {
                                c.emplace(ec,
                                          bytes_transferred);
                              });
    CHECK_FALSE(a);
    CHECK_FALSE(b);
    CHECK_FALSE(c);
    auto handlers = ctx.run();
    CHECK(handlers == 3);
    REQUIRE(a);
    CHECK_FALSE(a->first);
    CHECK(a->second == 5);
    REQUIRE(b);
    CHECK_FALSE(b->first);
    CHECK(b->second == 5);
    REQUIRE(c);
    CHECK_FALSE(c->first);
    CHECK(c->second == 2);
  }
  file = fd(::open(filename,
                   O_RDONLY));
  char buffer[16];
  auto result = ::read(file.native_handle(),
                       buffer,
                       sizeof(buffer));
  REQUIRE(result == 12);
  std::string_view sv(buffer,
                      12);
  CHECK(sv == str);
}

TEST_CASE("async_file unary async_flush",
          "[async_file]")
{
  std::string str("Hello world!");
  char filename[] = "/tmp/XXXXXX";
  fd file(::mkstemp(filename));
  INFO("Temporary file is " << filename);
  std::optional<std::error_code> ec;
  execution_context ctx(10);
  async_file async(ctx,
                   std::move(file));
  async.async_flush([&](auto e) noexcept { ec = e; });
  CHECK_FALSE(ec);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK_FALSE(*ec);
}

TEST_CASE("async_file binary async_flush",
          "[async_file]")
{
  std::string str("Hello world!");
  char filename[] = "/tmp/XXXXXX";
  fd file(::mkstemp(filename));
  INFO("Temporary file is " << filename);
  std::optional<std::error_code> a;
  std::optional<std::error_code> b;
  execution_context ctx(10);
  async_file async(ctx,
                   std::move(file));
  async.async_flush(true,
                    [&](auto e) noexcept { a = e; });
  async.async_flush(false,
                    [&](auto e) noexcept { b = e; });
  CHECK_FALSE(a);
  CHECK_FALSE(b);
  auto handlers = ctx.run();
  CHECK(handlers == 2);
  REQUIRE(a);
  CHECK_FALSE(*a);
  REQUIRE(b);
  CHECK_FALSE(*b);
}

TEST_CASE("async_file get_executor",
          "[async_file]")
{
  char filename[] = "/tmp/XXXXXX";
  fd file(::mkstemp(filename));
  INFO("Temporary file is " << filename);
  execution_context ctx(10);
  execution_context other_ctx(10);
  async_file async(ctx,
                   std::move(file));
  CHECK(async.get_executor() == ctx.get_executor());
  CHECK(async.get_executor() != other_ctx.get_executor());
}

}
}
