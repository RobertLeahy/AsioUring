#include <asio_uring/lockfree_node.hpp>

#include <cstddef>
#include <utility>
#include <boost/core/noncopyable.hpp>

#include <catch2/catch.hpp>

namespace asio_uring::tests {
namespace {

class state : private boost::noncopyable {
public:
  state() noexcept
    : construct(0),
      destruct (0)
  {}
  std::size_t construct;
  std::size_t destruct;
};

class object : private boost::noncopyable {
public:
  explicit object(state& s) noexcept
    : state_(s)
  {
    ++state_.construct;
  }
  ~object() noexcept {
    ++state_.destruct;
  }
private:
  state& state_;
};

TEST_CASE("lockfree_node emplace/reset",
          "[lockfree][lockfree_node]")
{
  state s;
  lockfree_node<object> node;
  node.emplace(s);
  CHECK(s.construct == 1);
  CHECK(s.destruct == 0);
  node.reset();
  CHECK(s.construct == 1);
  CHECK(s.destruct == 1);
}

TEST_CASE("lockfree_node get",
          "[lockfree][lockfree_node]")
{
  lockfree_node<int> node;
  node.emplace(5);
  CHECK(node.get() == 5);
  CHECK(std::as_const(node).get() == 5);
  node.reset();
}

}
}
