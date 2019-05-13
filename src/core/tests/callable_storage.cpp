#include <asio_uring/callable_storage.hpp>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <asio_uring/test/allocator.hpp>
#include <boost/core/noncopyable.hpp>

#include <catch2/catch.hpp>

namespace asio_uring::tests {
namespace {

class state : private boost::noncopyable{
public:
  state() noexcept
    : invoked(false),
      throws (false)
  {}
  bool invoked;
  bool throws;
};

class callable {
public:
  explicit callable(state& s) noexcept
    : s_(s)
  {}
  void operator()() {
    s_.invoked = true;
    if (s_.throws) {
      throw std::runtime_error("corge");
    }
  }
private:
  state& s_;
};

TEST_CASE("callable_storage small object",
          "[callable_storage]")
{
  state s;
  callable c(s);
  using allocator_type = test::allocator<callable>;
  allocator_type::state_type as;
  allocator_type a(as);
  {
    callable_storage<1024> storage(std::move(c),
                                   a);
    CHECK(as.allocate == 0);
    CHECK(as.deallocate == 0);
    CHECK(as.construct == 0);
    CHECK(as.destroy == 0);
    CHECK_FALSE(s.invoked);
    storage();
    CHECK(s.invoked);
  }
  CHECK(as.allocate == 0);
  CHECK(as.deallocate == 0);
  CHECK(as.construct == 0);
  CHECK(as.destroy == 0);
}

class big_state : public state {
public:
  big_state() noexcept
    : allocate  (0),
      deallocate(0),
      construct (0),
      destroy   (0)
  {}
  std::size_t allocate;
  std::size_t deallocate;
  std::size_t construct;
  std::size_t destroy;
};

class big_callable : public callable {
public:
  big_callable(big_state& state,
               test::allocator<big_callable>::state_type& alloc_state) noexcept
    : callable    (state),
      state_      (state),
      alloc_state_(alloc_state)
  {}
  void operator()() {
    callable& self = *this;
    self();
    state_.allocate = alloc_state_.allocate;
    state_.deallocate = alloc_state_.deallocate;
    state_.construct = alloc_state_.construct;
    state_.destroy = alloc_state_.destroy;
  }
private:
  big_state&                                 state_;
  test::allocator<big_callable>::state_type& alloc_state_;
  char                                       arr_[1024];
};

TEST_CASE("callable_storage big object",
          "[callable_storage]")
{
  big_state s;
  using allocator_type = test::allocator<big_callable>;
  allocator_type::state_type as;
  big_callable c(s,
                 as);
  allocator_type a(as);
  {
    callable_storage<1024> storage(std::move(c),
                                   a);
    CHECK(as.allocate == 1);
    CHECK(as.deallocate == 0);
    CHECK(as.construct == 1);
    CHECK(as.destroy == 0);
    CHECK_FALSE(s.invoked);
    storage();
    CHECK(s.invoked);
    CHECK(s.allocate == 1);
    CHECK(s.deallocate == 1);
    CHECK(s.construct == 1);
    CHECK(s.destroy == 1);
  }
  CHECK(as.allocate == 1);
  CHECK(as.deallocate == 1);
  CHECK(as.construct == 1);
  CHECK(as.destroy == 1);
}

class big_move_throws_state : public big_state {
public:
  big_move_throws_state() noexcept
    : move_throws(false)
  {}
  bool move_throws;
};

class big_callable_move_throws : public big_callable {
public:
  big_callable_move_throws(big_move_throws_state& state,
                           test::allocator<big_callable_move_throws>::state_type& alloc_state) noexcept
    : big_callable(state,
                   alloc_state),
      state_      (state)
  {}
  big_callable_move_throws(big_callable_move_throws&& rhs)
    : big_callable(rhs),
      state_      (rhs.state_)
  {
    if (state_.move_throws) {
      throw std::runtime_error("quux");
    }
  }
private:
  big_move_throws_state& state_;
};

TEST_CASE("callable_storage big object move ctor throws",
          "[callable_storage]")
{
  big_move_throws_state s;
  using allocator_type = test::allocator<big_callable_move_throws>;
  allocator_type::state_type as;
  big_callable_move_throws c(s,
                             as);
  allocator_type a(as);
  callable_storage<1024> storage(std::move(c),
                                 a);
  CHECK(as.allocate == 1);
  CHECK(as.deallocate == 0);
  CHECK(as.construct == 1);
  CHECK(as.destroy == 0);
  CHECK_FALSE(s.invoked);
  s.move_throws = true;
  CHECK_THROWS_AS(storage(),
                  std::runtime_error);
  CHECK_FALSE(s.invoked);
  CHECK(as.allocate == 1);
  CHECK(as.deallocate == 1);
  CHECK(as.construct == 1);
  CHECK(as.destroy == 1);
}

TEST_CASE("callable_storage big object throws",
          "[callable_storage]")
{
  big_state s;
  using allocator_type = test::allocator<big_callable>;
  allocator_type::state_type as;
  big_callable c(s,
                 as);
  allocator_type a(as);
  callable_storage<1024> storage(std::move(c),
                                 a);
  CHECK(as.allocate == 1);
  CHECK(as.deallocate == 0);
  CHECK(as.construct == 1);
  CHECK(as.destroy == 0);
  CHECK_FALSE(s.invoked);
  s.throws = true;
  CHECK_THROWS_AS(storage(),
                  std::runtime_error);
  CHECK(s.invoked);
  CHECK(as.allocate == 1);
  CHECK(as.deallocate == 1);
  CHECK(as.construct == 1);
  CHECK(as.destroy == 1);
}

}
}
