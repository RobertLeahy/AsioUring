/**
 *  \file
 */

#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <mutex>
#include <new>
#include <type_traits>
#include <utility>
#include "eventfd.hpp"
#include "spin_lock.hpp"

namespace asio_uring {

template<typename T,
         typename Allocator = std::allocator<T>>
class eventfd_queue : private Allocator {
public:
  using native_handle_type = eventfd::native_handle_type;
  using const_native_handle_type = eventfd::const_native_handle_type;
  using value_type = T;
  using allocator_type = Allocator;
  using integer_type = eventfd::integer_type;
private:
  using allocator_traits_type = std::allocator_traits<allocator_type>;
  using storage_type = std::aligned_union_t<1,
                                            value_type>;
  class node_type {
  public:
    node_type() noexcept
      : next(nullptr)
    {}
    node_type(const node_type&) = delete;
    node_type(node_type&&) = delete;
    node_type& operator=(const node_type&) = delete;
    node_type& operator=(node_type&&) = delete;
    node_type*   next;
    storage_type storage;
    void reset() noexcept {
      get().~value_type();
    }
    template<typename... Args>
    void emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<value_type,
                                                                          Args...>)
    {
      new(&storage) value_type(std::forward<Args>(args)...);
    }
    value_type& get() noexcept {
      return *reinterpret_cast<value_type*>(&storage);
    }
  };
  using rebound_allocator_type = typename allocator_traits_type::template rebind_alloc<node_type>;
  using rebound_allocator_traits_type = typename allocator_traits_type::template rebind_traits<node_type>;
public:
  explicit eventfd_queue(const allocator_type& alloc = allocator_type())
    : Allocator(alloc),
      head_    (nullptr),
      tail_    (nullptr),
      free_    (nullptr)
  {}
  eventfd_queue(const eventfd_queue&) = delete;
  eventfd_queue(eventfd_queue&&) = delete;
  eventfd_queue& operator=(const eventfd_queue&) = delete;
  eventfd_queue& operator=(eventfd_queue&&) = delete;
  ~eventfd_queue() noexcept {
    while (head_) {
      auto node = head_;
      head_ = head_->next;
      node->reset();
      destroy_node(node);
    }
    while (free_) {
      auto node = free_;
      free_ = free_->next;
      destroy_node(node);
    }
  }
  allocator_type get_allocator() const noexcept {
    return allocator_type(*this);
  }
  template<typename... Args>
  void emplace(Args&&... args) {
    auto&& node = get_node();
    assert(!node.next);
    try {
      node.emplace(std::forward<Args>(args)...);
    } catch (...) {
      release_node(node);
      throw;
    }
    {
      std::scoped_lock l(lock_);
      if (tail_) {
        assert(head_);
        tail_->next = &node;
      } else {
        assert(!head_);
        head_ = &node;
      }
      tail_ = &node;
    }
    e_.write(1);
  }
  void push(value_type v) {
    emplace(std::move(v));
  }
  integer_type pending() {
    return e_.read();
  }
  template<typename Function>
  void consume(integer_type n,
               Function f)
  {
    for (integer_type i = 0; i < n; ++i) {
      consume_one(f);
    }
  }
  template<typename Function>
  std::size_t consume_all(Function f) {
    auto to_read = pending();
    consume(to_read,
            std::move(f));
    return to_read;
  }
  native_handle_type native_handle() noexcept {
    return e_.native_handle();
  }
  const_native_handle_type native_handle() const noexcept {
    return e_.native_handle();
  }
private:
  template<typename Function>
  void consume_one(Function f) {
    auto&& node = pop_node();
    try {
      f(node.get());
    } catch (...) {
      node.reset();
      release_node(node);
      throw;
    }
    node.reset();
    release_node(node);
  }
  node_type& pop_node() noexcept {
    std::scoped_lock l(lock_);
    assert(head_);
    assert(tail_);
    auto&& retr = *head_;
    head_ = head_->next;
    if (!head_) {
      tail_ = nullptr;
    }
    return retr;
  }
  node_type& get_node() {
    {
      std::scoped_lock l(free_lock_);
      if (free_) {
        auto&& retr = *free_;
        free_ = free_->next;
        retr.next = nullptr;
        return retr;
      }
    }
    rebound_allocator_type alloc(get_allocator());
    node_type* ptr = rebound_allocator_traits_type::allocate(alloc,
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
    return *ptr;
  }
  void release_node(node_type& node) noexcept {
    std::scoped_lock l(free_lock_);
    node.next = free_;
    free_ = &node;
  }
  void destroy_node(node_type* ptr) noexcept {
    assert(ptr);
    rebound_allocator_type alloc(get_allocator());
    rebound_allocator_traits_type::destroy(alloc,
                                           ptr);
    rebound_allocator_traits_type::deallocate(alloc,
                                              ptr,
                                              1);
  }
  eventfd           e_;
  mutable spin_lock lock_;
  node_type*        head_;
  node_type*        tail_;
  mutable spin_lock free_lock_;
  node_type*        free_;
};

}
