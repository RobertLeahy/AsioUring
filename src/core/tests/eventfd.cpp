#include <asio_uring/eventfd.hpp>

#include <limits>
#include <optional>
#include <system_error>
#include <asio_uring/liburing.hpp>
#include <asio_uring/uring.hpp>
#include <errno.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/uio.h>

#include <catch2/catch.hpp>

namespace asio_uring::tests {
namespace {

TEST_CASE("eventfd bad flags",
          "[eventfd]")
{
  std::optional<std::error_code> ec;
  try {
    eventfd e(0,
              std::numeric_limits<int>::max());
  } catch (const std::system_error& ex) {
    ec = ex.code();
  }
  REQUIRE(ec);
  CHECK(ec->value() == EINVAL);
  CHECK(&ec->category() == &std::generic_category());
}

TEST_CASE("eventfd throwing interface",
          "[eventfd]")
{
  eventfd e;
  e.write(1);
  eventfd::integer_type result = e.read();
  CHECK(result == 1);
  e.write(2);
  result = e.read();
  CHECK(result == 2);
}

TEST_CASE("eventfd std::error_code interface",
          "[eventfd]")
{
  eventfd e;
  std::error_code ec(EAGAIN,
                     std::generic_category());
  e.write(1,
          ec);
  REQUIRE_FALSE(ec);
  ec.assign(EINVAL,
            std::generic_category());
  eventfd::integer_type result = e.read(ec);
  REQUIRE_FALSE(ec);
  CHECK(result == 1);
  e.write(2,
          ec);
  REQUIRE_FALSE(ec);
  result = e.read(ec);
  REQUIRE_FALSE(ec);
  CHECK(result == 2);
}

TEST_CASE("eventfd initval",
          "[eventfd]")
{
  eventfd e(100);
  eventfd::integer_type result = e.read();
  CHECK(result == 100);
}

TEST_CASE("eventfd semaphore",
          "[eventfd]")
{
  eventfd e(0,
            EFD_SEMAPHORE);
  e.write(3);
  eventfd::integer_type result = e.read();
  CHECK(result == 1);
  result = e.read();
  CHECK(result == 1);
  result = e.read();
  CHECK(result == 1);
}

TEST_CASE("eventfd & uring poll",
          "[eventfd][uring]")
{
  uring u(1);
  eventfd e;
  ::io_uring_sqe* sqe = ::io_uring_get_sqe(u.native_handle());
  REQUIRE(sqe);
  ::io_uring_prep_poll_add(sqe,
                           e.native_handle(),
                           POLLIN);
  sqe->user_data = 5;
  REQUIRE_FALSE(::io_uring_get_sqe(u.native_handle()));
  ::io_uring_submit(u.native_handle());
  ::io_uring_cqe* cqe;
  int result = ::io_uring_peek_cqe(u.native_handle(),
                                   &cqe);
  REQUIRE(result == 0);
  REQUIRE_FALSE(cqe);
  e.write(1);
  result = ::io_uring_peek_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result == 0);
  REQUIRE(cqe);
  CHECK(cqe->user_data == 5);
  REQUIRE(cqe->res >= 0);
  CHECK(e.read() == 1);
}

}
}
