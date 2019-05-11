/**
 *  \file
 */

#pragma once

#include <atomic>
#include <cassert>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include "lockfree_node_pool.hpp"

namespace asio_uring {

template<typename T,
         typename Allocator = std::allocator<T>>
class lockfree_queue {
private:
  using node_pool_type = lockfree_node_pool<T,
                                            Allocator>;
  using node_pool_guard_type = typename node_pool_type::guard;
  using node_type = typename node_pool_type::node_type;
  using node_guard_type = typename node_type::guard;
  using node_next_guard_type = typename node_type::next_guard;
public:
  using value_type = typename node_pool_type::value_type;
  using allocator_type = typename node_pool_type::allocator_type;
  explicit lockfree_queue(const allocator_type& alloc = allocator_type()) noexcept
    : pool_(alloc),
      head_(nullptr),
      tail_(nullptr)
  {}
  lockfree_queue(const lockfree_queue&) = delete;
  lockfree_queue(lockfree_queue&&) = delete;
  lockfree_queue& operator=(const lockfree_queue&) = delete;
  lockfree_queue& operator=(lockfree_queue&&) = delete;
  bool empty() const noexcept {
    return !head_.load(std::memory_order_relaxed);
  }
  std::optional<T> pop() noexcept(std::is_nothrow_move_constructible_v<T>) {
    auto ptr = head_.load(std::memory_order_acquire);
    while (ptr) {
      auto next = ptr->acquire();
      if (!next) {
        ptr = head_.load(std::memory_order_acquire);
        continue;
      }
      node_next_guard_type g(*ptr,
                             *next);
      if (!*next) {
        auto tmp = head_.load(std::memory_order_acquire);
        if (tmp != ptr) {
          ptr = tmp;
          continue;
        }
        if (!tail_.compare_exchange_weak(ptr,
                                         nullptr,
                                         std::memory_order_acq_rel,
                                         std::memory_order_relaxed))
        {
          ptr = head_.load(std::memory_order_acquire);
          continue;
        }
#ifdef NDEBUG
        head_.store(nullptr,
                    std::memory_order_release);
#else
        assert(head_.exchange(nullptr,
                              std::memory_order_acq_rel) == ptr);
#endif
        g.release();
        return pop_impl(*ptr);
      }
      if (!head_.compare_exchange_weak(ptr,
                                       *next,
                                       std::memory_order_release,
                                       std::memory_order_acquire))
      {
        continue;
      }
      g.release();
      return pop_impl(*ptr);
    }
    return std::nullopt;
  }
  template<typename... Args>
  void emplace(Args&&... args) {
    auto&& node = pool_.acquire();
    assert(!node.acquire());
    node_pool_guard_type g(pool_,
                           node);
    node.emplace(std::forward<Args>(args)...);
    g.release();
    node_next_guard_type next_g(node,
                                nullptr);
    auto ptr = tail_.load(std::memory_order_acquire);
    for (;;) {
      if (!ptr) {
        if (!tail_.compare_exchange_weak(ptr,
                                         &node,
                                         std::memory_order_release,
                                         std::memory_order_acquire))
        {
          continue;
        }
#ifdef NDEBUG
        head_.store(&node,
                    std::memory_order_release);
#else
        assert(!head_.exchange(&node,
                               std::memory_order_acq_rel));
#endif
        break;
      }
      auto next = ptr->acquire();
      if (!next) {
        continue;
      }
      node_next_guard_type g(*ptr,
                             *next);
      if (*next) {
        continue;
      }
      node_next_guard_type commit_g(*ptr,
                                    &node);
      if (!tail_.compare_exchange_weak(ptr,
                                       &node,
                                       std::memory_order_release,
                                       std::memory_order_acquire))
      {
        commit_g.release();
        continue;
      }
      g.release();
      break;
    }
  }
  void push(T obj) {
    emplace(std::move(obj));
  }
private:
  std::optional<T> pop_impl(node_type& node) noexcept(std::is_nothrow_move_constructible_v<T>) {
    node_pool_guard_type pool_g(pool_,
                                node);
    node_guard_type node_g(node);
    return std::optional(std::move(node.get()));
  }
  node_pool_type          pool_;
  std::atomic<node_type*> head_;
  std::atomic<node_type*> tail_;
};

}
