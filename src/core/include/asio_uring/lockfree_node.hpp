/**
 *  \file
 */

#pragma once

#include <atomic>
#include <cassert>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>

namespace asio_uring {

template<typename T>
class lockfree_node {
public:
  lockfree_node() noexcept
    : next(this)
  {}
  lockfree_node(const lockfree_node&) = delete;
  lockfree_node(lockfree_node&&) = delete;
  lockfree_node& operator=(const lockfree_node&) = delete;
  lockfree_node& operator=(lockfree_node&&) = delete;
  std::atomic<lockfree_node*> next;
  T& get() noexcept {
    return *reinterpret_cast<T*>(&storage_);
  }
  const T& get() const noexcept {
    return *reinterpret_cast<const T*>(&storage_);
  }
  template<typename... Args>
  T& emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T,
                                                                      Args...>)
  {
    return *new(&storage_) T(std::forward<Args>(args)...);
  }
  void reset() noexcept {
    get().~T();
  }
  std::optional<lockfree_node*> acquire() noexcept {
    auto next = this->next.exchange(this,
                                    std::memory_order_acq_rel);
    if (next == this) {
      return std::nullopt;
    }
    return next;
  }
  void release(lockfree_node* next) noexcept {
    assert(next != this);
#ifdef NDEBUG
    this->next.store(next,
                     std::memory_order_release);
#else
    assert(this->next.exchange(next,
                               std::memory_order_acq_rel) == this);
#endif
  }
  class guard {
  public:
    explicit guard(lockfree_node& self) noexcept
      : self_(&self)
    {}
    guard(const guard&) = delete;
    guard(guard&& other) noexcept
      : self_(other.self_)
    {
      other.self_ = nullptr;
    }
    guard& operator=(const guard&) = delete;
    guard& operator=(guard&& rhs) noexcept {
      assert(this != &rhs);
      destroy();
      self_ = rhs.self_;
      rhs.self_ = nullptr;
      return *this;
    }
    ~guard() noexcept {
      destroy();
    }
    void release() noexcept {
      self_ = nullptr;
    }
  private:
    void destroy() noexcept {
      if (self_) {
        self_->reset();
      }
    }
    lockfree_node* self_;
  };
  class next_guard {
  public:
    next_guard(lockfree_node& self,
               lockfree_node* next) noexcept
      :   self_(&self),
          next_(next)
    {}
    next_guard(const next_guard&) = delete;
    next_guard(next_guard&& other) noexcept
      : self_(other.self_),
        next_(other.next_)
    {
      other.self_ = nullptr;
    }
    next_guard& operator=(const next_guard&) = delete;
    next_guard& operator=(next_guard&& rhs) noexcept {
      assert(this != &rhs);
      destroy();
      self_ = rhs.self_;
      rhs.self_ = nullptr;
      return *this;
    }
    ~next_guard() noexcept {
      destroy();
    }
    void release() noexcept {
      self_ = nullptr;
    }
  private:
    void destroy() noexcept {
      if (self_) {
        self_->release(next_);
      }
    }
    lockfree_node* self_;
    lockfree_node* next_;
  };
private:
  using storage_type = std::aligned_union_t<1,
                                            T>;
  storage_type storage_;
};

}
