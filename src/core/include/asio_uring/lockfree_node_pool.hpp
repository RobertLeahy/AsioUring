/**
 *  \file
 */

#pragma once

#include <atomic>
#include <cassert>
#include <memory>
#include <type_traits>
#include "lockfree_node.hpp"

namespace asio_uring {

template<typename T,
         typename Allocator = std::allocator<T>>
class lockfree_node_pool : private Allocator {
public:
  using value_type = T;
  using allocator_type = Allocator;
  using node_type = lockfree_node<value_type>;
  using node_next_guard_type = typename node_type::next_guard;
private:
  using allocator_traits_type = std::allocator_traits<allocator_type>;
  using rebound_allocator_traits_type = typename allocator_traits_type::template rebind_traits<node_type>;
  using rebound_allocator_type = typename allocator_traits_type::template rebind_alloc<node_type>;
  static_assert(std::is_same_v<node_type*,
                               typename rebound_allocator_traits_type::pointer>);
public:
  explicit lockfree_node_pool(const allocator_type& alloc = allocator_type()) noexcept
    : Allocator(alloc),
      head_    (nullptr)
  {}
  lockfree_node_pool(const lockfree_node_pool&) = delete;
  lockfree_node_pool(lockfree_node_pool&&) = delete;
  lockfree_node_pool& operator=(const lockfree_node_pool&) = delete;
  lockfree_node_pool& operator=(lockfree_node_pool&&) = delete;
  ~lockfree_node_pool() noexcept {
    rebound_allocator_type alloc(get_allocator());
    auto ptr = head_.load(std::memory_order_relaxed);
    while (ptr) {
      auto next = ptr->next.load(std::memory_order_relaxed);
      assert(next != ptr);
      rebound_allocator_traits_type::destroy(alloc,
                                             ptr);
      rebound_allocator_traits_type::deallocate(alloc,
                                                ptr,
                                                1);
      ptr = next;
    }
  }
  allocator_type get_allocator() const noexcept {
    return allocator_type(*this);
  }
  node_type* try_acquire() noexcept {
    auto ptr = head_.load(std::memory_order_acquire);
    while (ptr) {
      auto next = ptr->acquire();
      if (!next) {
        ptr = head_.load(std::memory_order_acquire);
        continue;
      }
      node_next_guard_type g(*ptr,
                             *next);
      if (head_.compare_exchange_weak(ptr,
                                      *next,
                                      std::memory_order_release,
                                      std::memory_order_acquire))
      {
        g.release();
        return ptr;
      }
    }
    return nullptr;
  }
  node_type& allocate() {
    rebound_allocator_type alloc(get_allocator());
    auto ptr = rebound_allocator_traits_type::allocate(alloc,
                                                       1);
    try {
      rebound_allocator_traits_type::construct(alloc,
                                               ptr);
    } catch (...) {
      rebound_allocator_traits_type::deallocate(alloc,
                                                ptr,
                                                1);
      throw;
    }
    assert(!ptr->acquire());
    return *ptr;
  }
  node_type& acquire() {
    auto retr = try_acquire();
    if (retr) {
      return *retr;
    }
    return allocate();
  }
  void release(node_type& node) noexcept {
    assert(!node.acquire());
    auto head = head_.exchange(&node,
                               std::memory_order_acq_rel);
    node.release(head);
  }
  bool empty() const noexcept {
    return !head_.load(std::memory_order_relaxed);
  }
  class guard {
  public:
    guard() = delete;
    guard(const guard&) = delete;
    guard(guard&& other) noexcept
      : self_(other.self_),
        node_(other.node_)
    {
      other.self_ = nullptr;
      other.node_ = nullptr;
    }
    guard& operator=(const guard&) = delete;
    guard& operator=(guard&& rhs) noexcept {
      assert(this != &rhs);
      destroy();
      self_ = rhs.self_;
      node_ = rhs.node_;
      rhs.self_ = nullptr;
      rhs.node_ = nullptr;
      return *this;
    }
    guard(lockfree_node_pool& self,
          node_type& node) noexcept
      : self_(&self),
        node_(&node)
    {}
    ~guard() noexcept {
      destroy();
    }
    void release() noexcept {
      self_ = nullptr;
      node_ = nullptr;
    }
  private:
    void destroy() noexcept {
      if (self_) {
        assert(node_);
        self_->release(*node_);
      } else {
        assert(!node_);
      }
    }
    lockfree_node_pool* self_;
    node_type*          node_;
  };
private:
  std::atomic<node_type*> head_;
};

}
