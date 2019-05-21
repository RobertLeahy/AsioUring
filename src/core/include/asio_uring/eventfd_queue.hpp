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

/**
 *  A concurrent queue which uses an \ref eventfd
 *  as the notification mechanism.
 *
 *  \tparam T
 *    The element type.
 *  \tparam Allocator
 *    The `Allocator` type to use to customize the
 *    allocation strategy. Defaults to
 *    `std::allocator<T>`.
 */
template<typename T,
         typename Allocator = std::allocator<T>>
class eventfd_queue : private Allocator {
public:
  /**
   *  A type alias for \ref fd::native_handle_type.
   */
  using native_handle_type = eventfd::native_handle_type;
  /**
   *  A type alias for \ref fd::const_native_handle_type.
   */
  using const_native_handle_type = eventfd::const_native_handle_type;
  /**
   *  A type alias for the first template parameter to
   *  this class template.
   */
  using value_type = T;
  /**
   *  A type alias for the second template parameter to
   *  this class template.
   */
  using allocator_type = Allocator;
  /**
   *  A type alias for \ref eventfd::integer_type.
   */
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
  /**
   *  Creates an empty eventfd_queue which uses a
   *  certain instance of \ref allocator_type to
   *  allocate storage.
   *
   *  \param [in] alloc
   *    The instance of \ref allocator_type to use.
   *    Defaults to `allocator_type()` (i.e. a
   *    default constructed instance).
   */
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
#ifndef ASIO_URING_DOXYGEN_RUNNING
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
#endif
  /**
   *  Retrieves the specific instance of
   *  \ref allocator_type associated with this
   *  object.
   *
   *  \return
   *    The `Allocator`.
   */
  allocator_type get_allocator() const noexcept {
    return allocator_type(*this);
  }
  /**
   *  Publishes a value by:
   *
   *  1. Inserting it into the queue via emplace
   *     construction
   *  2. Calling \ref eventfd::write on the
   *     associated \ref eventfd.
   *
   *  Note that if \ref eventfd::write throws an
   *  exception the node remains in the queue.
   *
   *  \tparam Args
   *    The types of arguments to forward through
   *    to a constructor of \ref value_type.
   *
   *  \param [in] args
   *    The arguments to forward through to a
   *    constructor of \ref value_type.
   */
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
  /**
   *  Publishes a value by:
   *
   *  1. Inserting it into the queue by move
   *  2. Calling \ref eventfd::write on the
   *     associated \ref eventfd.
   *
   *  Note that if \ref eventfd::write throws an
   *  exception the node remains in the queue.
   *
   *  \param [in] v
   *    The value to publish.
   */
  void push(value_type v) {
    emplace(std::move(v));
  }
  /**
   *  Reads the count from the event file descriptor.
   *
   *  \warning
   *    \ref consume_all performs this internally.
   *    Calling this function clears the count associated
   *    with the event file descriptor and requires that
   *    the caller manage the count and make appropriate
   *    calls to \ref consume in order to consume objects
   *    (unless the returned value is `0`).
   *
   *  \return
   *    The count of the event file descriptor.
   */
  integer_type pending() {
    return e_.read();
  }
  /**
   *  Consumes objects from the queue.
   *
   *  \warning
   *    If this function is called with an `n` which is
   *    greater than the number of objects actually in
   *    the queue the result is undefined behavior. Therefore
   *    this function must only be used with the value
   *    returned by \ref pending or a value less than that
   *    value. If a value less than that value is used
   *    the caller must perform book keeping externally
   *    from this class to ensure that this function is
   *    never called with `n` arguments which sum to
   *    greater than the sum of all values ever returned
   *    by \ref pending.
   *
   *  \tparam Function
   *    An object which is invocable with signature
   *    `void(value_type)`.
   *
   *  \param [in] n
   *    The number of objects to consume.
   *  \param [in] f
   *    An object which shall be invoked with each
   *    dequeued object. If invocation of this object
   *    throws an exception the current object will have
   *    been consumed (as will all previous objects)
   *    but no subsequent objects.
   */
  template<typename Function>
  void consume(integer_type n,
               Function f)
  {
    for (integer_type i = 0; i < n; ++i) {
      consume_one(f);
    }
  }
  /**
   *  Consumes all objects currently in the queue.
   *
   *  \tparam Function
   *    An object which is invocable with signature
   *    `void(value_type)`.
   *
   *  \param [in] f
   *    An object which shall be invoked with each
   *    dequeued object. If invocation of this object
   *    throws an exception the current object will have
   *    been consumed (as will all previous objects)
   *    but no subsequent objects.
   *
   *  \return
   *    The number of objects dequeued.
   */
  template<typename Function>
  std::size_t consume_all(Function f) {
    auto to_read = pending();
    consume(to_read,
            std::move(f));
    return to_read;
  }
  /**
   *  @{
   *  Obtains the file descriptor of the associated
   *  \ref eventfd.
   *
   *  \return
   *    A file descriptor.
   */
  native_handle_type native_handle() noexcept {
    return e_.native_handle();
  }
  const_native_handle_type native_handle() const noexcept {
    return e_.native_handle();
  }
  /**
   *  @}
   */
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
