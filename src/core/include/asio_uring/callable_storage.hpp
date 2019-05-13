/**
 *  \file
 */

#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace asio_uring {

namespace detail::callable_storage {

class alignas(std::max_align_t) base {
public:
  base() = default;
  base(const base&) = default;
  base(base&&) = default;
  base& operator=(const base&) = default;
  base& operator=(base&&) = default;
  virtual ~base() noexcept;
  virtual void invoke() = 0;
};

class indirect_base {
public:
  indirect_base() = default;
  indirect_base(const indirect_base&) = default;
  indirect_base(indirect_base&&) = default;
  indirect_base& operator=(const indirect_base&) = default;
  indirect_base& operator=(indirect_base&&) = default;
  virtual void invoke() = 0;
  virtual void destroy() noexcept = 0;
};

template<typename T,
         typename Allocator>
class allocator_indirect final : public indirect_base,
                                 private Allocator
{
public:
  allocator_indirect(T t,
                     const Allocator& alloc) noexcept(std::is_nothrow_move_constructible_v<T>)
    : Allocator(alloc),
      t_       (std::move(t))
  {}
  virtual void invoke() override {
    prepare_invoke()();
  }
  virtual void destroy() noexcept override {
    Allocator alloc(*this);
    using traits_type = std::allocator_traits<Allocator>;
    using rebound_allocator_type = typename traits_type::template rebind_alloc<allocator_indirect>;
    using rebound_traits_type = typename traits_type::template rebind_traits<allocator_indirect>;
    rebound_allocator_type rebound(alloc);
    rebound_traits_type::destroy(rebound,
                                 this);
    rebound_traits_type::deallocate(rebound,
                                    this,
                                    1);
  }
private:
  T prepare_invoke() noexcept(std::is_nothrow_constructible_v<T>) {
    class guard {
    public:
      explicit guard(allocator_indirect& self) noexcept
        : self_(self)
      {}
      guard(const guard&) = delete;
      guard(guard&&) = delete;
      guard& operator=(const guard&) = delete;
      guard& operator=(guard&&) = delete;
      ~guard() noexcept {
        self_.destroy();
      }
    private:
      allocator_indirect& self_;
    };
    guard g(*this);
    return std::move(t_);
  }
  T t_;
};

class indirect final : public base {
private:
  template<typename T,
           typename Allocator>
  static indirect_base* create(T t,
                               const Allocator& alloc)
  {
    using traits_type = std::allocator_traits<Allocator>;
    using indirect_type = allocator_indirect<T,
                                             Allocator>;
    using rebound_allocator_type = typename traits_type::template rebind_alloc<indirect_type>;
    using rebound_traits_type = typename traits_type::template rebind_traits<indirect_type>;
    rebound_allocator_type rebound(alloc);
    indirect_type* retr = rebound_traits_type::allocate(rebound,
                                                        1);
    try {
      rebound_traits_type::construct(rebound,
                                     retr,
                                     std::move(t),
                                     alloc);
    } catch (...) {
      rebound_traits_type::deallocate(rebound,
                                      retr,
                                      1);
      throw;
    }
    return retr;
  }
public:
  template<typename T,
           typename Allocator>
  indirect(T t,
           const Allocator& alloc)
    : inner_(create(std::move(t),
                    alloc))
  {}
  ~indirect() noexcept;
  virtual void invoke() override;
private:
  indirect_base* inner_;
};

template<typename T>
class direct final : public base {
public:
  template<typename Allocator>
  direct(T t,
         const Allocator&) noexcept(std::is_nothrow_move_constructible_v<T>)
    : t_(std::move(t))
  {}
  virtual void invoke() override {
    t_();
  }
private:
  T t_;
};

template<typename Storage,
         typename T,
         typename Allocator>
class select {
private:
  using direct_type = direct<T>;
  using align_type = std::conditional_t<(alignof(direct_type) > alignof(base)),
                                        indirect,
                                        direct_type>;
public:
  using type = std::conditional_t<(sizeof(direct_type) > sizeof(Storage)),
                                  indirect,
                                  align_type>;
};

template<typename Storage,
         typename T,
         typename Allocator>
using select_t = typename select<Storage,
                                 T,
                                 Allocator>::type;

}

template<std::size_t Buffer>
class callable_storage {
public:
  callable_storage() = delete;
  callable_storage(const callable_storage&) = delete;
  callable_storage(callable_storage&&) = delete;
  callable_storage& operator=(const callable_storage&) = delete;
  callable_storage& operator=(callable_storage&&) = delete;
  template<typename T,
           typename Allocator>
  callable_storage(T&& t,
                   const Allocator& alloc)
  {
    auto ptr = new(&storage_) detail::callable_storage::select_t<storage_type,
                                                                 std::decay_t<T>,
                                                                 Allocator>(std::forward<T>(t),
                                                                            alloc);
#ifdef NDEBUG
    (void)ptr;
#else
    ptr_ = ptr;
#endif
  }
  ~callable_storage() noexcept {
    using type = detail::callable_storage::base;
    get().~type();
  }
  void operator()() {
    get().invoke();
  }
private:
  detail::callable_storage::base& get() noexcept {
    auto ptr = reinterpret_cast<detail::callable_storage::base*>(&storage_);
    assert(ptr_);
    assert(ptr == ptr_);
    return *ptr;
  }
  const detail::callable_storage::base& get() const noexcept {
    auto ptr = reinterpret_cast<detail::callable_storage::base*>(&storage_);
    assert(ptr_);
    assert(ptr == ptr_);
    return *ptr;
  }
  using storage_type = std::aligned_union_t<Buffer,
                                            detail::callable_storage::indirect>;
  storage_type                    storage_;
#ifndef NDEBUG
  detail::callable_storage::base* ptr_;
#endif
};

}
