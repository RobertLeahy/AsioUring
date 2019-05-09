#include <asio_uring/eventfd.hpp>

#include <limits>
#include <optional>
#include <system_error>
#include <errno.h>
#include <sys/eventfd.h>

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

}
}
