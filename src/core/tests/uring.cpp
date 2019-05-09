#include <asio_uring/uring.hpp>

#include <optional>
#include <system_error>
#include <type_traits>

#include <catch2/catch.hpp>

namespace asio_uring::tests {
namespace {

static_assert(!std::is_default_constructible_v<uring>);
static_assert(!std::is_move_constructible_v<uring>);
static_assert(!std::is_move_assignable_v<uring>);
static_assert(!std::is_copy_constructible_v<uring>);
static_assert(!std::is_copy_assignable_v<uring>);

TEST_CASE("uring",
          "[uring]")
{
  uring ring(1);
  REQUIRE(ring.native_handle());
  CHECK(ring.native_handle()->ring_fd != -1);
}

TEST_CASE("uring no entries",
          "[uring]")
{
  std::optional<std::error_code> ec;
  try {
    uring ring(0);
  } catch (const std::system_error& ex) {
    ec = ex.code();
  }
  REQUIRE(ec);
  CHECK(ec->value() != 0);
  CHECK(&ec->category() == &std::generic_category());
}

}
}
