#include <asio_uring/lockfree_node_pool.hpp>

#include <cstddef>
#include <optional>
#include <thread>
#include <vector>
#include <asio_uring/test/allocator.hpp>
#include <boost/core/noncopyable.hpp>

#include <catch2/catch.hpp>

namespace asio_uring::tests {
namespace {

TEST_CASE("lockfree_node_pool try_acquire",
          "[lockfree_node_pool][lockfree]")
{
  using pool_type = lockfree_node_pool<int>;
  pool_type pool;
  CHECK(pool.empty());
  auto ptr = pool.try_acquire();
  std::optional<pool_type::guard> g;
  if (ptr) {
    g.emplace(pool,
              *ptr);
  }
  CHECK(pool.empty());
  REQUIRE_FALSE(ptr);
  pool.release(pool.allocate());
  CHECK_FALSE(pool.empty());
  ptr = pool.try_acquire();
  std::optional<pool_type::guard> g2;
  if (ptr) {
    g2.emplace(pool,
               *ptr);
  }
  CHECK(ptr);
  CHECK(pool.empty());
}

TEST_CASE("lockfree_node_pool acquire & dtor",
          "[lockfree_node_pool][lockfree]")
{
  using allocator_type = test::allocator<int>;
  allocator_type::state_type s;
  allocator_type a(s);
  using pool_type = lockfree_node_pool<int,
                                       allocator_type>;
  {
    pool_type pool(a);
    CHECK(pool.empty());
    {
      auto&& node = pool.acquire();
      pool_type::guard g(pool,
                         node);
      CHECK(pool.empty());
      CHECK(s.allocate == 1);
      CHECK(s.construct == 1);
      CHECK(s.deallocate == 0);
      CHECK(s.destroy == 0);
    }
    CHECK_FALSE(pool.empty());
    CHECK(s.allocate == 1);
    CHECK(s.construct == 1);
    CHECK(s.deallocate == 0);
    CHECK(s.destroy == 0);
  }
  CHECK(s.allocate == 1);
  CHECK(s.construct == 1);
  CHECK(s.deallocate == 1);
  CHECK(s.destroy == 1);
}

TEST_CASE("lockfree_node_pool threading",
          "[lockfree_node_pool][lockfree]")
{
  using allocator_type = test::allocator<int>;
  allocator_type::state_type s;
  allocator_type a(s);
  using pool_type = lockfree_node_pool<int,
                                       allocator_type>;
  {
    pool_type pool(a);
    {
      using threads_type = std::vector<std::thread>;
      threads_type ts;
      class guard : private boost::noncopyable {
      public:
        explicit guard(threads_type& ts) noexcept
          : ts_(ts)
        {}
        ~guard() noexcept {
          while (!ts_.empty()) {
            ts_.back().join();
            ts_.pop_back();
          }
        }
      private:
        threads_type& ts_;
      };
      guard g(ts);
      while (ts.size() != 10) {
        ts.emplace_back([&]() {
          for (std::size_t i = 0; i < 100000; ++i) {
            if (!(i % 100)) {
              std::this_thread::yield();
            }
            pool.release(pool.acquire());
          }
        });
      }
    }
    CHECK_FALSE(pool.empty());
    CHECK(s.allocate != 0);
    CHECK(s.construct == s.allocate);
    CHECK(s.deallocate == 0);
    CHECK(s.destroy == 0);
  }
  CHECK(s.allocate != 0);
  CHECK(s.construct == s.allocate);
  CHECK(s.deallocate == s.allocate);
  CHECK(s.destroy == s.deallocate);
}

TEST_CASE("lockfree_node_pool allocator allocate throws",
          "[lockfree_node_pool][lockfree]")
{
  using allocator_type = test::allocator<int>;
  allocator_type::state_type s;
  allocator_type a(s);
  using pool_type = lockfree_node_pool<int,
                                       allocator_type>;
  pool_type pool(a);
  s.allocate_throws = true;
  CHECK_THROWS_AS(pool.release(pool.acquire()),
                  allocator_type::allocate_exception_type);
  CHECK(pool.empty());
  CHECK(s.allocate == 1);
  CHECK(s.construct == 0);
  CHECK(s.deallocate == 0);
  CHECK(s.destroy == 0);
}

TEST_CASE("lockfree_node_pool allocator construct throws",
          "[lockfree_node_pool][lockfree]")
{
  using allocator_type = test::allocator<int>;
  allocator_type::state_type s;
  allocator_type a(s);
  using pool_type = lockfree_node_pool<int,
                                       allocator_type>;
  pool_type pool(a);
  s.construct_throws = true;
  CHECK_THROWS_AS(pool.release(pool.acquire()),
                  allocator_type::construct_exception_type);
  CHECK(pool.empty());
  CHECK(s.allocate == 1);
  CHECK(s.construct == 1);
  CHECK(s.deallocate == 1);
  CHECK(s.destroy == 0);
}

}
}
