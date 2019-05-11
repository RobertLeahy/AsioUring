#include <asio_uring/spin_lock.hpp>

#include <mutex>

#include <catch2/catch.hpp>

namespace asio_uring::tests {
namespace {

TEST_CASE("spin_lock",
          "[spin_lock]")
{
  spin_lock sl;
  std::unique_lock l(sl);
  REQUIRE(l.owns_lock());
  std::unique_lock l2(sl,
                      std::try_to_lock);
  CHECK_FALSE(l2.owns_lock());
  l.unlock();
  CHECK(l2.try_lock());
}

}
}
