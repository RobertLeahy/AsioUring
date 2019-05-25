#include <asio_uring/asio/service.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <system_error>
#include <asio_uring/asio/execution_context.hpp>
#include <asio_uring/fd.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/execution_context.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/system/error_code.hpp>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>

#include <catch2/catch.hpp>

namespace asio_uring::asio::tests {
namespace {

class guard : private boost::noncopyable {
public:
  guard(service& svc,
        service::implementation_type& impl) noexcept
    : svc_ (svc),
      impl_(impl)
  {}
  ~guard() noexcept {
    svc_.destroy(impl_);
  }
private:
  service&                      svc_;
  service::implementation_type& impl_;
};

TEST_CASE("service initiate_read_some_at eof",
          "[service]")
{
  char filename[] = "/tmp/XXXXXX";
  fd file(::mkstemp(filename));
  INFO("Temporary file is " << filename);
  file = fd();
  file = fd(::open(filename,
                   O_RDONLY));
  std::optional<std::error_code> ec;
  std::optional<std::size_t> bytes_transferred;
  char c;
  execution_context ctx(10);
  auto&& svc = boost::asio::use_service<service>(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  svc.initiate_read_some_at(impl,
                            file.native_handle(),
                            0,
                            boost::asio::mutable_buffer(&c,
                                                        sizeof(c)),
                            [&](auto e,
                                auto b) noexcept
                            {
                              ec = e;
                              bytes_transferred = b;
                            });
  CHECK_FALSE(ec);
  CHECK_FALSE(bytes_transferred);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK_FALSE(*ec);
  REQUIRE(bytes_transferred);
  CHECK(*bytes_transferred == 0);
}

TEST_CASE("service initiate_read_some_at full read",
          "[service]")
{
  std::string str("hello");
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
  std::optional<std::error_code> ec;
  std::optional<std::size_t> bytes_transferred;
  char buffer[5];
  execution_context ctx(10);
  auto&& svc = boost::asio::use_service<service>(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  svc.initiate_read_some_at(impl,
                            file.native_handle(),
                            0,
                            boost::asio::buffer(buffer),
                            [&](auto e,
                                auto b) noexcept
                            {
                              ec = e;
                              bytes_transferred = b;
                            });
  CHECK_FALSE(ec);
  CHECK_FALSE(bytes_transferred);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK_FALSE(*ec);
  REQUIRE(bytes_transferred);
  REQUIRE(*bytes_transferred == sizeof(buffer));
  CHECK(std::equal(str.begin(),
                   str.end(),
                   std::begin(buffer),
                   std::end(buffer)));
}

TEST_CASE("service initiate_read_some_at partial read",
          "[service]")
{
  std::string str("hello");
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
  std::optional<std::error_code> ec;
  std::optional<std::size_t> bytes_transferred;
  char buffer[10];
  execution_context ctx(10);
  auto&& svc = boost::asio::use_service<service>(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  svc.initiate_read_some_at(impl,
                            file.native_handle(),
                            0,
                            boost::asio::buffer(buffer),
                            [&](auto e,
                                auto b) noexcept
                            {
                              ec = e;
                              bytes_transferred = b;
                            });
  CHECK_FALSE(ec);
  CHECK_FALSE(bytes_transferred);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK_FALSE(*ec);
  REQUIRE(bytes_transferred);
  REQUIRE(*bytes_transferred == str.size());
  CHECK(std::equal(str.begin(),
                   str.end(),
                   buffer,
                   buffer + str.size()));
}

TEST_CASE("service initiate_read_some_at bad handle") {
  std::optional<std::error_code> ec;
  std::optional<std::size_t> bytes_transferred;
  char c;
  execution_context ctx(10);
  auto&& svc = boost::asio::use_service<service>(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  svc.initiate_read_some_at(impl,
                            -1,
                            0,
                            boost::asio::mutable_buffer(&c,
                                                        sizeof(c)),
                            [&](auto e,
                                auto b) noexcept
                            {
                              ec = e;
                              bytes_transferred = b;
                            });
  CHECK_FALSE(ec);
  CHECK_FALSE(bytes_transferred);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK(*ec);
  CHECK(*ec == boost::system::error_code(EBADF,
                                         boost::system::generic_category()));
  REQUIRE(bytes_transferred);
  CHECK(*bytes_transferred == 0);
}

TEST_CASE("service initiate_read_some_at offset",
          "[service]")
{
  std::string str("hello world");
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
  std::optional<std::error_code> ec;
  std::optional<std::size_t> bytes_transferred;
  char buffer[10];
  execution_context ctx(10);
  auto&& svc = boost::asio::use_service<service>(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  svc.initiate_read_some_at(impl,
                            file.native_handle(),
                            6,
                            boost::asio::buffer(buffer),
                            [&](auto e,
                                auto b) noexcept
                            {
                              ec = e;
                              bytes_transferred = b;
                            });
  CHECK_FALSE(ec);
  CHECK_FALSE(bytes_transferred);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK_FALSE(*ec);
  REQUIRE(bytes_transferred);
  REQUIRE(*bytes_transferred == 5);
  CHECK(std::equal(str.begin() + 6,
                   str.end(),
                   buffer,
                   buffer + 5));
}

TEST_CASE("service initiate_write_some_at",
          "[service]")
{
  char filename[] = "/tmp/XXXXXX";
  fd file(::mkstemp(filename));
  INFO("Temporary file is " << filename);
  std::optional<std::error_code> ec;
  std::optional<std::size_t> bytes_transferred;
  const std::string str("hello");
  execution_context ctx(10);
  auto&& svc = boost::asio::use_service<service>(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  svc.initiate_write_some_at(impl,
                             file.native_handle(),
                             0,
                             boost::asio::buffer(str),
                             [&](auto e,
                                 auto b) noexcept
                             {
                               ec = e;
                               bytes_transferred = b;
                             });
  CHECK_FALSE(ec);
  CHECK_FALSE(bytes_transferred);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  {
    INFO(ec->message());
    CHECK_FALSE(*ec);
  }
  REQUIRE(bytes_transferred);
  CHECK(*bytes_transferred == str.size());
  file = fd();
  file = fd(::open(filename,
                   O_RDONLY));
  char buffer[5];
  auto read = ::read(file.native_handle(),
                     buffer,
                     sizeof(buffer));
  REQUIRE(read == sizeof(buffer));
  CHECK(std::equal(std::begin(buffer),
                   std::end(buffer),
                   str.begin(),
                   str.end()));
  char c;
  read = ::read(file.native_handle(),
                &c,
                sizeof(c));
  CHECK(read == 0);
}

TEST_CASE("service initiate_write_some_at multiple simultaneous & offset",
          "[service]")
{
  char filename[] = "/tmp/XXXXXX";
  fd file(::mkstemp(filename));
  INFO("Temporary file is " << filename);
  std::optional<std::error_code> ec;
  std::optional<std::size_t> bytes_transferred;
  std::optional<std::error_code> ec_2;
  std::optional<std::size_t> bytes_transferred_2;
  const std::string str("hello world");
  execution_context ctx(10);
  auto&& svc = boost::asio::use_service<service>(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  svc.initiate_write_some_at(impl,
                             file.native_handle(),
                             0,
                             boost::asio::const_buffer(str.data(),
                                                       5),
                             [&](auto e,
                                 auto b) noexcept
                             {
                               ec = e;
                               bytes_transferred = b;
                             });
  svc.initiate_write_some_at(impl,
                             file.native_handle(),
                             5,
                             boost::asio::const_buffer(str.data() + 5,
                                                       str.size() - 5),
                             [&](auto e,
                                 auto b) noexcept
                             {
                               ec_2 = e;
                               bytes_transferred_2= b;
                             });
  CHECK_FALSE(ec);
  CHECK_FALSE(bytes_transferred);
  CHECK_FALSE(ec_2);
  CHECK_FALSE(bytes_transferred_2);
  auto handlers = ctx.run();
  CHECK(handlers == 2);
  REQUIRE(ec);
  {
    INFO(ec->message());
    CHECK_FALSE(*ec);
  }
  REQUIRE(bytes_transferred);
  CHECK(*bytes_transferred == 5);
  REQUIRE(ec_2);
  {
    INFO(ec_2->message());
    CHECK_FALSE(*ec_2);
  }
  REQUIRE(bytes_transferred_2);
  CHECK(*bytes_transferred_2 == 6);
  file = fd();
  file = fd(::open(filename,
                   O_RDONLY));
  char buffer[11];
  auto read = ::read(file.native_handle(),
                     buffer,
                     sizeof(buffer));
  REQUIRE(read == sizeof(buffer));
  CHECK(std::equal(std::begin(buffer),
                   std::end(buffer),
                   str.begin(),
                   str.end()));
  char c;
  read = ::read(file.native_handle(),
                &c,
                sizeof(c));
  CHECK(read == 0);
}

TEST_CASE("service initiate_write_some_at bad handle",
          "[service]")
{
  std::optional<std::error_code> ec;
  std::optional<std::size_t> bytes_transferred;
  char c;
  execution_context ctx(10);
  auto&& svc = boost::asio::use_service<service>(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  svc.initiate_write_some_at(impl,
                             -1,
                             0,
                             boost::asio::mutable_buffer(&c,
                                                         sizeof(c)),
                             [&](auto e,
                                 auto b) noexcept
                             {
                               ec = e;
                               bytes_transferred = b;
                             });
  CHECK_FALSE(ec);
  CHECK_FALSE(bytes_transferred);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK(*ec);
  CHECK(*ec == boost::system::error_code(EBADF,
                                         boost::system::generic_category()));
  REQUIRE(bytes_transferred);
  CHECK(*bytes_transferred == 0);
}

TEST_CASE("service initiate_poll_add",
          "[service]")
{
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  std::optional<std::error_code> ec;
  execution_context ctx(10);
  auto&& svc = boost::asio::use_service<service>(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  svc.initiate_poll_add(impl,
                        read.native_handle(),
                        POLLIN,
                        [&](auto e) noexcept { ec = e; });
  CHECK_FALSE(ec);
  auto handlers = ctx.poll();
  CHECK(handlers == 0);
  CHECK_FALSE(ec);
  ctx.restart();
  char c = 'A';
  auto written = ::write(write.native_handle(),
                         &c,
                         sizeof(c));
  REQUIRE(written == sizeof(c));
  handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK_FALSE(*ec);
}

TEST_CASE("service initiate_poll_add bad handle",
          "[service]")
{
  std::optional<std::error_code> ec;
  execution_context ctx(10);
  auto&& svc = boost::asio::use_service<service>(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  svc.initiate_poll_add(impl,
                        -1,
                        POLLIN,
                        [&](auto e) noexcept { ec = e; });
  CHECK_FALSE(ec);
  auto handlers = ctx.poll();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK(*ec);
  CHECK(*ec == boost::system::error_code(EBADF,
                                         boost::system::generic_category()));
}

TEST_CASE("service initiate_poll_remove",
          "[service]")
{
  int pipes[2];
  auto result = ::pipe(pipes);
  REQUIRE(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  std::optional<std::error_code> ec;
  std::optional<std::error_code> remove_ec;
  execution_context ctx(10);
  auto&& svc = boost::asio::use_service<service>(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  svc.initiate_poll_add(impl,
                        read.native_handle(),
                        POLLIN,
                        [&](auto e) noexcept { ec = e; });
  CHECK_FALSE(ec);
  auto handlers = ctx.poll();
  CHECK(handlers == 0);
  CHECK_FALSE(ec);
  ctx.restart();
  REQUIRE(std::distance(impl.begin(),
                        impl.end()) == 1);
  svc.initiate_poll_remove(impl,
                           *impl.begin(),
                           [&](auto e) noexcept { remove_ec = e; });
  CHECK_FALSE(ec);
  CHECK_FALSE(remove_ec);
  handlers = ctx.run_one();
  CHECK(handlers == 1);
  ctx.restart();
  CHECK_FALSE(ec);
  REQUIRE(remove_ec);
  CHECK_FALSE(*remove_ec);
  handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK(*ec);
  std::error_code c(make_error_code(boost::asio::error::operation_aborted));
  CHECK(*ec == c);
}

TEST_CASE("service initiate_poll_remove not found",
          "[service]")
{
  std::optional<std::error_code> ec;
  execution_context ctx(10);
  auto&& svc = boost::asio::use_service<service>(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  svc.initiate_poll_remove(impl,
                           nullptr,
                           [&](auto e) noexcept { ec = e; });
  CHECK_FALSE(ec);
  auto handlers = ctx.run_one();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK(*ec);
  CHECK(*ec == boost::system::error_code(ENOENT,
                                         boost::system::generic_category()));
}

TEST_CASE("service initiate_fsync",
          "[service]")
{
  char filename[] = "/tmp/XXXXXX";
  fd file(::mkstemp(filename));
  INFO("Temporary file is " << filename);
  std::optional<std::error_code> ec;
  execution_context ctx(10);
  auto&& svc = boost::asio::use_service<service>(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  svc.initiate_fsync(impl,
                     file.native_handle(),
                     false,
                     [&](auto e) noexcept { ec = e; });
  CHECK_FALSE(ec);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK_FALSE(*ec);
}

TEST_CASE("service initiate_fsync fdatasync",
          "[service]")
{
  char filename[] = "/tmp/XXXXXX";
  fd file(::mkstemp(filename));
  INFO("Temporary file is " << filename);
  std::optional<std::error_code> ec;
  execution_context ctx(10);
  auto&& svc = boost::asio::use_service<service>(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  svc.initiate_fsync(impl,
                     file.native_handle(),
                     true,
                     [&](auto e) noexcept { ec = e; });
  CHECK_FALSE(ec);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK_FALSE(*ec);
}

TEST_CASE("service initiate_fsync bad fd",
          "[service]")
{
  std::optional<std::error_code> ec;
  execution_context ctx(10);
  auto&& svc = boost::asio::use_service<service>(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  svc.initiate_fsync(impl,
                     -1,
                     false,
                     [&](auto e) noexcept { ec = e; });
  CHECK_FALSE(ec);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(ec);
  CHECK(*ec);
  CHECK(*ec == boost::system::error_code(EBADF,
                                         boost::system::generic_category()));
}

}
}
