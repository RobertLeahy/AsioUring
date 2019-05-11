#include <asio_uring/lockfree_queue.hpp>

#include <atomic>
#include <thread>
#include <vector>
#include <asio_uring/test/allocator.hpp>

#include <catch2/catch.hpp>

namespace asio_uring::tests {
namespace {

TEST_CASE("lockfree_queue emplace & pop",
          "[lockfree_queue][lockfree]")
{
  lockfree_queue<int> queue;
  CHECK(queue.empty());
  CHECK_FALSE(queue.pop());
  queue.emplace(5);
  CHECK_FALSE(queue.empty());
  queue.emplace(4);
  CHECK_FALSE(queue.empty());
  auto opt = queue.pop();
  CHECK_FALSE(queue.empty());
  REQUIRE(opt);
  CHECK(*opt == 5);
  opt = queue.pop();
  CHECK(queue.empty());
  REQUIRE(opt);
  CHECK(*opt == 4);
  CHECK_FALSE(queue.pop());
}

TEST_CASE("lockfree_queue push & pop",
          "[lockfree_queue][lockfree]")
{
  using allocator_type = test::allocator<int>;
  allocator_type::state_type s;
  allocator_type a(s);
  {
    lockfree_queue<int,
                   allocator_type> queue(a);
    CHECK(queue.empty());
    CHECK_FALSE(queue.pop());
    CHECK(s.allocate == 0);
    CHECK(s.deallocate == 0);
    CHECK(s.construct == 0);
    CHECK(s.destroy == 0);
    queue.push(5);
    CHECK_FALSE(queue.empty());
    CHECK(s.allocate == 1);
    CHECK(s.deallocate == 0);
    CHECK(s.construct == 1);
    CHECK(s.destroy == 0);
    queue.push(4);
    CHECK(s.allocate == 2);
    CHECK(s.deallocate == 0);
    CHECK(s.construct == 2);
    CHECK(s.destroy == 0);
    CHECK_FALSE(queue.empty());
    auto opt = queue.pop();
    CHECK_FALSE(queue.empty());
    CHECK(s.allocate == 2);
    CHECK(s.deallocate == 0);
    CHECK(s.construct == 2);
    CHECK(s.destroy == 0);
    REQUIRE(opt);
    CHECK(*opt == 5);
    opt = queue.pop();
    CHECK(queue.empty());
    CHECK(s.allocate == 2);
    CHECK(s.deallocate == 0);
    CHECK(s.construct == 2);
    CHECK(s.destroy == 0);
    REQUIRE(opt);
    CHECK(*opt == 4);
    CHECK_FALSE(queue.pop());
  }
  CHECK(s.allocate == 2);
  CHECK(s.deallocate == 2);
  CHECK(s.construct == 2);
  CHECK(s.destroy == 2);
}

//TEST_CASE("lockfree_queue threading",
//          "[lockfree_queue][lockfree]")
//{
//  using allocator_type = test::allocator<int>;
//  allocator_type::state_type s;
//  allocator_type a(s);
//  {
//    lockfree_queue<int,
//                   allocator_type> queue(a);
//    {
//      using threads_type = std::vector<std::thread>;
//      threads_type ts;
//      class guard : private boost::noncopyable {
//      public:
//        explicit guard(threads_type& ts,
//                       std::atomic<bool>& done) noexcept
//          : ts_  (ts),
//            done_(done)
//        {}
//        ~guard() noexcept {
//          done_ = true;
//          while (!ts_.empty()) {
//            ts_.back().join();
//            ts_.pop_back();
//          }
//        }
//      private:
//        threads_type&      ts_;
//        std::atomic<bool>& done_;
//      };
//      std::atomic<std::size_t> produced(0);
//      std::atomic<std::size_t> consumed(0);
//      std::atomic<bool> done(false);
//      guard g(ts,
//              done);
//      std::size_t expected = 0;
//      while (ts.size() != 20) {
//        ts.emplace_back([&]() {
//          for (std::size_t i = 0; i < 100000; ++i) {
//            queue.push(1);
//            ++produced;
//          }
//        });
//        expected += 100000;
//        ts.emplace_back([&]() {
//          for (;;) {
//            auto opt = queue.pop();
//            if (opt) {
//              ++consumed;
//              continue;
//            }
//            if (produced == consumed) {
//              if (done.load()) {
//                return;
//              }
//            }
//          }
//        });
//      }
//      while (produced != expected);
//    }
//  }
//  CHECK(s.allocate != 0);
//  CHECK(s.construct == s.allocate);
//  CHECK(s.deallocate == s.allocate);
//  CHECK(s.destroy == s.deallocate);
//}

}
}
