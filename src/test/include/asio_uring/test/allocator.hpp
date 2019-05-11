/**
 *  \file
 */

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <utility>
#include <boost/core/noncopyable.hpp>

namespace asio_uring::test {

namespace detail {

class allocator_state : private boost::noncopyable {
public:
  allocator_state() noexcept;
  std::atomic<std::size_t> allocate;
  std::atomic<std::size_t> deallocate;
  std::atomic<std::size_t> construct;
  std::atomic<std::size_t> destroy;
  bool                     allocate_throws;
  bool                     construct_throws;
};

}

template<typename T>
class allocator {
public:
  using allocate_exception_type = std::bad_alloc;
  using construct_exception_type = std::runtime_error;
  using state_type = detail::allocator_state;
  explicit allocator(state_type& s) noexcept
    : s_(&s)
  {}
  template<typename U>
  allocator(const allocator<U>& other) noexcept
    : s_(&other.get_state())
  {}
  allocator& operator=(const allocator& rhs) noexcept {
    s_ = rhs.s_;
    return *this;
  }
  state_type& get_state() const noexcept {
    assert(s_);
    return *s_;
  }
  using value_type = T;
  using size_type = std::size_t;
  value_type* allocate(size_type n) {
    assert(s_);
    s_->allocate.fetch_add(1,
                           std::memory_order_relaxed);
    if (s_->allocate_throws) {
      throw std::bad_alloc();
    }
    if (n != 1) {
      throw std::bad_alloc();
    }
    auto retr = reinterpret_cast<value_type*>(std::malloc(sizeof(T)));
    if (!retr) {
      throw std::bad_alloc();
    }
    return retr;
  }
  void deallocate(value_type* ptr,
                  size_type n) noexcept
  {
    assert(s_);
    assert(ptr);
    assert(n == 1);
    s_->deallocate.fetch_add(1,
                             std::memory_order_relaxed);
    std::free(ptr);
  }
  template<typename... Args>
  void construct(void* ptr,
                 Args&&... args)
  {
    assert(s_);
    assert(ptr);
    s_->construct.fetch_add(1,
                            std::memory_order_relaxed);
    if (s_->construct_throws) {
      throw std::runtime_error("construct");
    }
    new(ptr) T(std::forward<Args>(args)...);
  }
  void destroy(void* ptr) noexcept {
    assert(s_);
    assert(ptr);
    s_->destroy.fetch_add(1,
                          std::memory_order_relaxed);
    static_cast<value_type*>(ptr)->~value_type();
  }
private:
  state_type* s_;
};

}
