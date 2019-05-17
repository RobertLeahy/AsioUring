#include <asio_uring/service.hpp>

#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <asio_uring/execution_context.hpp>
#include <asio_uring/fd.hpp>
#include <asio_uring/liburing.hpp>
#include <boost/core/noncopyable.hpp>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>

#include <catch2/catch.hpp>

namespace asio_uring::tests {
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

TEST_CASE("service initiate",
          "[service]")
{
  void* initial_data = nullptr;
  std::optional<::io_uring_cqe> cqe;
  execution_context ctx(100);
  service svc(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  CHECK(impl.begin() == impl.end());
  std::allocator<void> a;
  svc.initiate(impl,
               [&](auto&& sqe,
                   auto data) noexcept
               {
                 ::io_uring_prep_nop(&sqe);
                 initial_data = data;
               },
               [&](auto c) { cqe = c; },
               a);
  CHECK(initial_data);
  CHECK_FALSE(cqe);
  CHECK(std::distance(impl.begin(),
                      impl.end()) == 1);
  auto handlers = ctx.poll();
  CHECK(handlers == 1);
  REQUIRE(cqe);
  CHECK(::io_uring_cqe_get_data(&*cqe) == initial_data);
  CHECK(impl.begin() == impl.end());
  void* second_data = nullptr;
  cqe = std::nullopt;
  svc.initiate(impl,
               [&](auto&& sqe,
                   auto data) noexcept
               {
                 ::io_uring_prep_nop(&sqe);
                 second_data = data;
               },
               [&](auto c) { cqe = c; },
               a);
  CHECK(second_data == initial_data);
  CHECK_FALSE(cqe);
  CHECK(std::distance(impl.begin(),
                      impl.end()) == 1);
  handlers = ctx.poll();
  CHECK(handlers == 0);
  ctx.restart();
  handlers = ctx.poll();
  CHECK(handlers == 1);
  REQUIRE(cqe);
  CHECK(::io_uring_cqe_get_data(&*cqe) == second_data);
  CHECK(impl.begin() == impl.end());
}

TEST_CASE("service poll add/remove",
          "[service]")
{
  int pipes[2];
  auto result = ::pipe(pipes);
  CHECK(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  void* user_data = nullptr;
  std::optional<::io_uring_cqe> add_cqe;
  std::optional<::io_uring_cqe> remove_cqe;
  execution_context ctx(100);
  service svc(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  CHECK(impl.begin() == impl.end());
  std::allocator<void> a;
  svc.initiate(impl,
               [&](auto&& sqe,
                   auto data) noexcept
               {
                 ::io_uring_prep_poll_add(&sqe,
                                          read.native_handle(),
                                          POLLIN);
                 user_data = data;
               },
               [&](auto c) { add_cqe = c; },
               a);
  CHECK(std::distance(impl.begin(),
                      impl.end()) == 1);
  CHECK(user_data);
  CHECK_FALSE(add_cqe);
  auto handlers = ctx.poll();
  CHECK(handlers == 0);
  CHECK_FALSE(add_cqe);
  CHECK(std::distance(impl.begin(),
                      impl.end()) == 1);
  svc.initiate(impl,
               [&](auto&& sqe,
                   auto data) noexcept
               {
                 ::io_uring_prep_poll_remove(&sqe,
                                             user_data);
               },
               [&](auto c) { remove_cqe = c; },
               a);
  CHECK_FALSE(add_cqe);
  CHECK_FALSE(remove_cqe);
  CHECK(std::distance(impl.begin(),
                      impl.end()) == 2);
  handlers = ctx.poll();
  CHECK(handlers == 0);
  CHECK_FALSE(add_cqe);
  CHECK_FALSE(remove_cqe);
  ctx.restart();
  handlers = ctx.poll();
  CHECK(handlers == 2);
  REQUIRE(add_cqe);
  CHECK(add_cqe->res == 0);
  REQUIRE(remove_cqe);
  CHECK(remove_cqe->res == 0);
  CHECK(impl.begin() == impl.end());
}

TEST_CASE("service move_construct",
          "[service]")
{
  execution_context ctx(100);
  service svc(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  std::allocator<void> a;
  svc.initiate(impl,
               [&](auto&& sqe,
                   auto data) noexcept
               {
                 ::io_uring_prep_nop(&sqe);
               },
               [&](auto) {},
               a);
  CHECK(std::distance(impl.begin(),
                      impl.end()) == 1);
  service::implementation_type impl_2;
  svc.move_construct(impl_2,
                     impl);
  CHECK(std::distance(impl_2.begin(),
                      impl_2.end()) == 1);
}

TEST_CASE("service move_assign",
          "[service]")
{
  execution_context ctx(100);
  service svc(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  std::allocator<void> a;
  svc.initiate(impl,
               [&](auto&& sqe,
                   auto data) noexcept
               {
                 ::io_uring_prep_nop(&sqe);
               },
               [&](auto) {},
               a);
  CHECK(std::distance(impl.begin(),
                      impl.end()) == 1);
  service::implementation_type impl_2;
  svc.construct(impl_2);
  svc.initiate(impl_2,
               [&](auto&& sqe,
                   auto data) noexcept
               {
                 ::io_uring_prep_nop(&sqe);
               },
               [&](auto) {},
               a);
  CHECK(std::distance(impl_2.begin(),
                      impl_2.end()) == 1);
  svc.initiate(impl_2,
               [&](auto&& sqe,
                   auto data) noexcept
               {
                 ::io_uring_prep_nop(&sqe);
               },
               [&](auto) {},
               a);
  CHECK(std::distance(impl_2.begin(),
                      impl_2.end()) == 2);
  svc.move_assign(impl,
                  svc,
                  impl_2);
  CHECK(std::distance(impl.begin(),
                      impl.end()) == 2);
}

TEST_CASE("service initiate w/iovs",
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
  char buffer[5];
  void* initial_data = nullptr;
  void* initial_iovs = nullptr;
  void* second_data = nullptr;
  void* second_iovs = nullptr;
  std::optional<::io_uring_cqe> cqe;
  execution_context ctx(100);
  service svc(ctx);
  service::implementation_type impl;
  svc.construct(impl);
  guard g(svc,
          impl);
  std::allocator<void> a;
  svc.initiate(impl,
               1,
               [&](auto&& sqe,
                   auto* iovs,
                   auto data) noexcept
               {
                 iovs->iov_base = buffer;
                 iovs->iov_len = sizeof(buffer);
                 initial_data = data;
                 initial_iovs = iovs;
                 ::io_uring_prep_readv(&sqe,
                                       file.native_handle(),
                                       iovs,
                                       1,
                                       0);
               },
               [&](auto&& c) { cqe = c; },
               a);
  CHECK(initial_data);
  CHECK(initial_iovs);
  CHECK_FALSE(cqe);
  auto handlers = ctx.run();
  CHECK(handlers == 1);
  REQUIRE(cqe);
  REQUIRE(cqe->res == 5);
  CHECK(::io_uring_cqe_get_data(&*cqe) == initial_data);
  CHECK(std::equal(std::begin(buffer),
                   std::end(buffer),
                   str.begin(),
                   str.end()));
  cqe = std::nullopt;
  svc.initiate(impl,
               1,
               [&](auto&& sqe,
                   auto* iovs,
                   auto data) noexcept
               {
                 iovs->iov_base = buffer;
                 iovs->iov_len = sizeof(buffer);
                 second_data = data;
                 second_iovs = iovs;
                 ::io_uring_prep_readv(&sqe,
                                       file.native_handle(),
                                       iovs,
                                       1,
                                       0);
               },
               [&](auto&& c) { cqe = c; },
               a);
  CHECK(second_data);
  CHECK(second_data == initial_data);
  CHECK(second_iovs);
  CHECK(second_iovs == initial_iovs);
  CHECK_FALSE(cqe);
}

}
}
