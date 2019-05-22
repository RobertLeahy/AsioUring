#include <asio_uring/asio/poll_file.hpp>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <system_error>
#include <utility>
#include <vector>
#include <asio_uring/asio/execution_context.hpp>
#include <asio_uring/fd.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <catch2/catch.hpp>

namespace asio_uring::asio::tests {
namespace {

TEST_CASE("poll_file async_poll_in",
          "[poll_file]")
{
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  std::optional<std::error_code> ec;
  execution_context ctx(10);
  poll_file poll(ctx,
                 std::move(read));
  poll.async_poll_in([&](auto e) noexcept { ec = e; });
  CHECK_FALSE(ec);
  auto handlers = ctx.poll();
  CHECK(handlers == 0);
  CHECK_FALSE(ec);
  char c = 'A';
  auto written = ::write(write.native_handle(),
                         &c,
                         sizeof(c));
  REQUIRE(written == 1);
  ctx.restart();
  handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK_FALSE(*ec);
}

TEST_CASE("poll_file async_poll_out",
          "[poll_file]")
{
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  std::optional<std::error_code> ec;
  execution_context ctx(10);
  poll_file poll(ctx,
                 std::move(write));
  poll.async_poll_out([&](auto e) noexcept { ec = e; });
  CHECK_FALSE(ec);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK_FALSE(*ec);
}

TEST_CASE("poll_file get_executor",
          "[poll_file]")
{
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  execution_context ctx(10);
  execution_context other_ctx(10);
  poll_file poll(ctx,
                 std::move(write));
  CHECK(poll.get_executor() == ctx.get_executor());
  CHECK(poll.get_executor() != other_ctx.get_executor());
}

TEST_CASE("poll_file async_read_some",
          "[poll_file]")
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
  using pair_type = std::pair<std::error_code,
                              std::size_t>;
  std::optional<pair_type> o;
  execution_context ctx(10);
  poll_file poll(ctx,
                 std::move(read));
  char buffer[16];
  poll.async_read_some(boost::asio::buffer(buffer),
                       [&](auto ec,
                           auto bytes_transferred) noexcept
                       {
                         o.emplace(ec,
                                   bytes_transferred);
                       });
  CHECK_FALSE(o);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(o);
  REQUIRE_FALSE(o->first);
  REQUIRE(o->second == sv.size());
  std::string_view sv2(buffer,
                       o->second);
  CHECK(sv == sv2);
}

TEST_CASE("poll_file async_read_some empty buffer sequence",
          "[poll_file]")
{
  int pipes[2];
  auto result = ::pipe2(pipes,
                        O_NONBLOCK);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  using pair_type = std::pair<std::error_code,
                              std::size_t>;
  std::optional<pair_type> o;
  execution_context ctx(10);
  poll_file poll(ctx,
                 std::move(read));
  boost::asio::mutable_buffer buffer(nullptr,
                                     0);
  poll.async_read_some(buffer,
                       [&](auto ec,
                           auto bytes_transferred) noexcept
                       {
                         o.emplace(ec,
                                   bytes_transferred);
                       });
  CHECK_FALSE(o);
  auto handlers = ctx.poll();
  CHECK(handlers == 1);
  REQUIRE(o);
  CHECK_FALSE(o->first);
  CHECK(o->second == 0);
}

TEST_CASE("poll_file async_read_some customize executor",
          "[poll_file]")
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
  using pair_type = std::pair<std::error_code,
                              std::size_t>;
  std::optional<pair_type> o;
  execution_context ctx(10);
  execution_context other_ctx(10);
  poll_file poll(ctx,
                 std::move(read));
  char buffer[16];
  auto bound = boost::asio::bind_executor(other_ctx.get_executor(),
                                          [&](auto ec,
                                              auto bytes_transferred) noexcept
                                          {
                                            o.emplace(ec,
                                                      bytes_transferred);
                                          });
  poll.async_read_some(boost::asio::buffer(buffer),
                       bound);
  CHECK_FALSE(o);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  CHECK_FALSE(o);
  handlers = other_ctx.run();
  CHECK(handlers == 1);
  REQUIRE(o);
  REQUIRE_FALSE(o->first);
  REQUIRE(o->second == sv.size());
  std::string_view sv2(buffer,
                       o->second);
  CHECK(sv == sv2);
}

TEST_CASE("poll_file async_read_some empty buffer sequence customize executor",
          "[poll_file]")
{
  int pipes[2];
  auto result = ::pipe2(pipes,
                        O_NONBLOCK);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  using pair_type = std::pair<std::error_code,
                              std::size_t>;
  std::optional<pair_type> o;
  execution_context other_ctx(10);
  execution_context ctx(10);
  poll_file poll(ctx,
                 std::move(read));
  boost::asio::mutable_buffer buffer(nullptr,
                                     0);
  auto bound = boost::asio::bind_executor(other_ctx.get_executor(),
                                          [&](auto ec,
                                              auto bytes_transferred) noexcept
                                          {
                                            o.emplace(ec,
                                                      bytes_transferred);
                                          });
  poll.async_read_some(buffer,
                       bound);
  CHECK_FALSE(o);
  auto handlers = ctx.poll();
  CHECK(handlers == 0);
  CHECK_FALSE(o);
  handlers = other_ctx.poll();
  CHECK(handlers == 1);
  REQUIRE(o);
  CHECK_FALSE(o->first);
  CHECK(o->second == 0);
}

TEST_CASE("poll_file async_write_some",
          "[poll_file]")
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
  vec.push_back(0);
  unsigned char start = 0;
  std::iota(vec.begin(),
            vec.end(),
            start);
  using pair_type = std::pair<std::error_code,
                              std::size_t>;
  std::optional<pair_type> o;
  execution_context ctx(10);
  poll_file poll(ctx,
                 std::move(write));
  auto func = [&](auto ec,
                  auto bytes_transferred) noexcept
              {
                o.emplace(ec,
                          bytes_transferred);
              };
  poll.async_write_some(boost::asio::buffer(vec),
                        func);
  CHECK_FALSE(o);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(o);
  REQUIRE_FALSE(o->first);
  REQUIRE(o->second == size);
  CHECK(o->second != vec.size());
  o = std::nullopt;
  poll.async_write_some(boost::asio::const_buffer(vec.data() + size,
                                                  1),
                        func);
  CHECK_FALSE(o);
  ctx.restart();
  handlers = ctx.poll();
  CHECK(handlers == 0);
  std::vector<unsigned char> vec2;
  vec2.resize(size);
  auto bytes = ::read(read.native_handle(),
                      vec2.data(),
                      vec2.size());
  REQUIRE(bytes == size);
  CHECK(std::equal(vec2.begin(),
                   vec2.end(),
                   vec.begin(),
                   vec.end() - 1));
  ctx.restart();
  handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(o);
  REQUIRE_FALSE(o->first);
  REQUIRE(o->second == 1);
  char c;
  bytes = ::read(read.native_handle(),
                 &c,
                 sizeof(c));
  REQUIRE(bytes == sizeof(c));
  CHECK(c == vec.back());
}

TEST_CASE("poll_file async_write_some empty buffer sequence",
          "[poll_file]")
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
  unsigned char start = 0;
  std::iota(vec.begin(),
            vec.end(),
            start);
  auto written = ::write(write.native_handle(),
                         vec.data(),
                         vec.size());
  REQUIRE(written == size);
  char c = 'A';
  written = ::write(write.native_handle(),
                    &c,
                    sizeof(c));
  REQUIRE(written == -1);
  REQUIRE(((errno == EAGAIN) ||
           (errno == EWOULDBLOCK)));
  using pair_type = std::pair<std::error_code,
                              std::size_t>;
  std::optional<pair_type> o;
  execution_context ctx(10);
  poll_file poll(ctx,
                 std::move(write));
  poll.async_write_some(boost::asio::const_buffer(nullptr,
                                                  0),
                        [&](auto ec,
                            auto bytes_transferred) noexcept
                        {
                          o.emplace(ec,
                                    bytes_transferred);
                        });
  CHECK_FALSE(o);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(o);
  CHECK_FALSE(o->first);
  CHECK(o->second == 0);
}

TEST_CASE("poll_file async_read_some boost::system::error_code",
          "[poll_file]")
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
  using pair_type = std::pair<boost::system::error_code,
                              std::size_t>;
  std::optional<pair_type> o;
  execution_context ctx(10);
  poll_file poll(ctx,
                 std::move(read));
  char buffer[16];
  poll.async_read_some(boost::asio::buffer(buffer),
                       [&](boost::system::error_code ec,
                           auto bytes_transferred) noexcept
                       {
                         o.emplace(ec,
                                   bytes_transferred);
                       });
  CHECK_FALSE(o);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(o);
  REQUIRE_FALSE(o->first);
  REQUIRE(o->second == sv.size());
  std::string_view sv2(buffer,
                       o->second);
  CHECK(sv == sv2);
}

TEST_CASE("poll_file async_write_some boost::system::error_code",
          "[poll_file]")
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
  vec.push_back(0);
  unsigned char start = 0;
  std::iota(vec.begin(),
            vec.end(),
            start);
  using pair_type = std::pair<boost::system::error_code,
                              std::size_t>;
  std::optional<pair_type> o;
  execution_context ctx(10);
  poll_file poll(ctx,
                 std::move(write));
  auto func = [&](boost::system::error_code ec,
                  auto bytes_transferred) noexcept
              {
                o.emplace(ec,
                          bytes_transferred);
              };
  poll.async_write_some(boost::asio::buffer(vec),
                        func);
  CHECK_FALSE(o);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(o);
  REQUIRE_FALSE(o->first);
  REQUIRE(o->second == size);
  CHECK(o->second != vec.size());
  o = std::nullopt;
  poll.async_write_some(boost::asio::const_buffer(vec.data() + size,
                                                  1),
                        func);
  CHECK_FALSE(o);
  ctx.restart();
  handlers = ctx.poll();
  CHECK(handlers == 0);
  std::vector<unsigned char> vec2;
  vec2.resize(size);
  auto bytes = ::read(read.native_handle(),
                      vec2.data(),
                      vec2.size());
  REQUIRE(bytes == size);
  CHECK(std::equal(vec2.begin(),
                   vec2.end(),
                   vec.begin(),
                   vec.end() - 1));
  ctx.restart();
  handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(o);
  REQUIRE_FALSE(o->first);
  REQUIRE(o->second == 1);
  char c;
  bytes = ::read(read.native_handle(),
                 &c,
                 sizeof(c));
  REQUIRE(bytes == sizeof(c));
  CHECK(c == vec.back());
}

}
}
