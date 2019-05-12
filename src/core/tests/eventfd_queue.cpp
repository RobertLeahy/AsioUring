#include <asio_uring/eventfd_queue.hpp>

#include <atomic>
#include <stdexcept>
#include <thread>
#include <vector>
#include <asio_uring/eventfd.hpp>
#include <asio_uring/liburing.hpp>
#include <asio_uring/uring.hpp>
#include <asio_uring/test/allocator.hpp>
#include <boost/core/noncopyable.hpp>
#include <poll.h>
#include <sys/uio.h>

#include <iostream>

#include <catch2/catch.hpp>

namespace asio_uring::tests {
namespace {

TEST_CASE("eventfd_queue emplace & consume_all",
          "[eventfd_queue][eventfd]")
{
  std::vector<int> consumed;
  eventfd_queue<int> queue;
  queue.emplace(1);
  queue.emplace(2);
  queue.consume_all([&](auto i) { consumed.push_back(i); });
  auto iter = consumed.begin();
  REQUIRE_FALSE(iter == consumed.end());
  CHECK(*iter == 1);
  ++iter;
  REQUIRE_FALSE(iter == consumed.end());
  CHECK(*iter == 2);
  ++iter;
  CHECK(iter == consumed.end());
  queue.emplace(3);
  queue.emplace(4);
  queue.emplace(5);
  queue.consume_all([&](auto i) { consumed.push_back(i); });
  iter = consumed.begin();
  REQUIRE_FALSE(iter == consumed.end());
  CHECK(*iter == 1);
  ++iter;
  REQUIRE_FALSE(iter == consumed.end());
  CHECK(*iter == 2);
  ++iter;
  REQUIRE_FALSE(iter == consumed.end());
  CHECK(*iter == 3);
  ++iter;
  REQUIRE_FALSE(iter == consumed.end());
  CHECK(*iter == 4);
  ++iter;
  REQUIRE_FALSE(iter == consumed.end());
  CHECK(*iter == 5);
  ++iter;
  CHECK(iter == consumed.end());
}

TEST_CASE("eventfd_queue push & consume_all",
          "[eventfd_queue][eventfd]")
{
  using allocator_type = test::allocator<int>;
  allocator_type::state_type s;
  allocator_type a(s);
  std::vector<int> consumed;
  {
    eventfd_queue<int,
                  allocator_type> queue(a);
    queue.push(1);
    CHECK(s.allocate == 1);
    CHECK(s.construct == 1);
    CHECK(s.deallocate == 0);
    CHECK(s.destroy == 0);
    queue.push(2);
    CHECK(s.allocate == 2);
    CHECK(s.construct == 2);
    CHECK(s.deallocate == 0);
    CHECK(s.destroy == 0);
    queue.consume_all([&](auto i) { consumed.push_back(i); });
    CHECK(s.allocate == 2);
    CHECK(s.construct == 2);
    CHECK(s.deallocate == 0);
    CHECK(s.destroy == 0);
    auto iter = consumed.begin();
    REQUIRE_FALSE(iter == consumed.end());
    CHECK(*iter == 1);
    ++iter;
    REQUIRE_FALSE(iter == consumed.end());
    CHECK(*iter == 2);
    ++iter;
    CHECK(iter == consumed.end());
  }
  CHECK(s.allocate == 2);
  CHECK(s.construct == 2);
  CHECK(s.deallocate == 2);
  CHECK(s.destroy == 2);
}

TEST_CASE("eventfd_queue pending",
          "[eventfd_queue][eventfd]")
{
  using allocator_type = test::allocator<int>;
  allocator_type::state_type s;
  allocator_type a(s);
  std::vector<int> consumed;
  {
    eventfd_queue<int,
                  allocator_type> queue(a);
    queue.emplace(1);
    CHECK(s.allocate == 1);
    CHECK(s.construct == 1);
    CHECK(s.deallocate == 0);
    CHECK(s.destroy == 0);
    queue.emplace(2);
    CHECK(s.allocate == 2);
    CHECK(s.construct == 2);
    CHECK(s.deallocate == 0);
    CHECK(s.destroy == 0);
    auto pending = queue.pending();
    REQUIRE(pending == 2);
    queue.consume(1,
                  [&](auto i) { consumed.push_back(i); });
    CHECK(s.allocate == 2);
    CHECK(s.construct == 2);
    CHECK(s.deallocate == 0);
    CHECK(s.destroy == 0);
    auto iter = consumed.begin();
    REQUIRE_FALSE(iter == consumed.end());
    CHECK(*iter == 1);
    ++iter;
    CHECK(iter == consumed.end());
    queue.consume(1,
                  [&](auto i) { consumed.push_back(i); });
    CHECK(s.allocate == 2);
    CHECK(s.construct == 2);
    CHECK(s.deallocate == 0);
    CHECK(s.destroy == 0);
    iter = consumed.begin();
    REQUIRE_FALSE(iter == consumed.end());
    CHECK(*iter == 1);
    ++iter;
    REQUIRE_FALSE(iter == consumed.end());
    CHECK(*iter == 2);
    ++iter;
    CHECK(iter == consumed.end());
  }
  CHECK(s.allocate == 2);
  CHECK(s.construct == 2);
  CHECK(s.deallocate == 2);
  CHECK(s.destroy == 2);
}

TEST_CASE("eventfd_queue exception",
          "[eventfd_queue][eventfd]")
{
  using allocator_type = test::allocator<int>;
  allocator_type::state_type s;
  allocator_type a(s);
  std::vector<int> consumed;
  {
    eventfd_queue<int,
                  allocator_type> queue(a);
    queue.emplace(1);
    CHECK(s.allocate == 1);
    CHECK(s.construct == 1);
    CHECK(s.deallocate == 0);
    CHECK(s.destroy == 0);
    queue.emplace(2);
    CHECK(s.allocate == 2);
    CHECK(s.construct == 2);
    CHECK(s.deallocate == 0);
    CHECK(s.destroy == 0);
    auto pending = queue.pending();
    REQUIRE(pending == 2);
    auto func = [&](auto i) {
      consumed.push_back(i);
      throw std::runtime_error("foo");
    };
    CHECK_THROWS_AS(queue.consume(1,
                                  func),
                    std::runtime_error);
    CHECK(s.allocate == 2);
    CHECK(s.construct == 2);
    CHECK(s.deallocate == 0);
    CHECK(s.destroy == 0);
    auto iter = consumed.begin();
    REQUIRE_FALSE(iter == consumed.end());
    CHECK(*iter == 1);
    ++iter;
    CHECK(iter == consumed.end());
    queue.consume(1,
                  [&](auto i) { consumed.push_back(i); });
    CHECK(s.allocate == 2);
    CHECK(s.construct == 2);
    CHECK(s.deallocate == 0);
    CHECK(s.destroy == 0);
    iter = consumed.begin();
    REQUIRE_FALSE(iter == consumed.end());
    CHECK(*iter == 1);
    ++iter;
    REQUIRE_FALSE(iter == consumed.end());
    CHECK(*iter == 2);
    ++iter;
    CHECK(iter == consumed.end());
  }
  CHECK(s.allocate == 2);
  CHECK(s.construct == 2);
  CHECK(s.deallocate == 2);
  CHECK(s.destroy == 2);
}

TEST_CASE("eventfd & uring polling",
          "[eventfd_queue][eventfd]")
{
  uring u(2);
  eventfd e;
  using allocator_type = test::allocator<int>;
  allocator_type::state_type s;
  allocator_type a(s);
  {
    eventfd_queue<int,
                  allocator_type> queue(a);
    std::atomic<std::size_t> produced(0);
    std::atomic<std::size_t> consumed(0);
    std::thread t([&]() {
      ::io_uring_sqe* sqe = ::io_uring_get_sqe(u.native_handle());
      CHECK(sqe);
      if (!sqe) {
        return;
      }
      ::io_uring_prep_poll_add(sqe,
                               e.native_handle(),
                               POLLIN);
      for (;;) {
        ::io_uring_sqe* sqe = ::io_uring_get_sqe(u.native_handle());
        CHECK(sqe);
        if (!sqe) {
          return;
        }
        ::io_uring_prep_poll_add(sqe,
                                 queue.native_handle(),
                                 POLLIN);
        ::io_uring_submit(u.native_handle());
        ::io_uring_cqe* cqe;
        int result = ::io_uring_wait_cqe(u.native_handle(),
                                         &cqe);
        CHECK(result >= 0);
        if (result < 0) {
          return;
        }
        if (!cqe->user_data) {
          return;
        }
        consumed += queue.consume_all([&](auto&&) {});
        if (consumed == produced) {
          return;
        }
        ::io_uring_cqe_seen(u.native_handle(),
                            cqe);
      }
    });
    class guard : private boost::noncopyable {
    public:
      guard(std::thread& t,
            eventfd& done) noexcept
        : t_   (t),
          done_(done)
      {}
      ~guard() noexcept {
        done_.write(1);
        t_.join();
      }
    private:
      std::thread& t_;
      eventfd&     done_;
    };
    guard g(t,
            e);
    for (std::size_t i = 0; i < 100000; ++i) {
      queue.emplace(1);
    }
    while (produced != consumed);
  }
  CHECK(s.allocate != 0);
  CHECK(s.construct == s.allocate);
  CHECK(s.deallocate == s.allocate);
  CHECK(s.destroy == s.construct);
}

}
}
