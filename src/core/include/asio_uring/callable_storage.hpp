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

template<typename R,
         typename... Args>
class alignas(std::max_align_t) base {
public:
  base() = default;
  base(const base&) = default;
  base(base&&) = default;
  base& operator=(const base&) = default;
  base& operator=(base&&) = default;
  virtual ~base() noexcept {}
  virtual R invoke(Args...) = 0;
};

template<typename F,
         typename... Args>
class indirect_base {
public:
  indirect_base() = default;
  indirect_base(const indirect_base&) = default;
  indirect_base(indirect_base&&) = default;
  indirect_base& operator=(const indirect_base&) = default;
  indirect_base& operator=(indirect_base&&) = default;
  virtual F invoke(Args...) = 0;
  virtual void destroy() noexcept = 0;
};

template<typename T,
         typename Allocator,
         typename R,
         typename... Args>
class allocator_indirect final : public indirect_base<R,
                                                      Args...>,
                                 private Allocator
{
public:
  allocator_indirect(T t,
                     const Allocator& alloc) noexcept(std::is_nothrow_move_constructible_v<T>)
    : Allocator(alloc),
      t_       (std::move(t))
  {}
  virtual R invoke(Args... args) override {
    return prepare_invoke()(std::forward<Args>(args)...);
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

template<typename R,
         typename... Args>
class indirect final : public base<R,
                                   Args...>
{
private:
  using indirect_base_type = indirect_base<R,
                                           Args...>;
  template<typename T,
           typename Allocator>
  static indirect_base_type* create(T t,
                                    const Allocator& alloc)
  {
    using traits_type = std::allocator_traits<Allocator>;
    using indirect_type = allocator_indirect<T,
                                             Allocator,
                                             R,
                                             Args...>;
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
  ~indirect() noexcept {
    if (inner_) {
      inner_->destroy();
    }
  }
  virtual R invoke(Args...) override {
    auto ptr = inner_;
    assert(ptr);
    inner_ = nullptr;
    return ptr->invoke();
  }
private:
  indirect_base_type* inner_;
};

template<typename T,
         typename R,
         typename... Args>
class direct final : public base<R,
                                 Args...>
{
public:
  template<typename Allocator>
  direct(T t,
         const Allocator&) noexcept(std::is_nothrow_move_constructible_v<T>)
    : t_(std::move(t))
  {}
  virtual R invoke(Args... args) override {
    return t_(std::forward<Args>(args)...);
  }
private:
  T t_;
};

template<typename Storage,
         typename T,
         typename Allocator,
         typename R,
         typename... Args>
class select {
private:
  using direct_type = direct<T,
                             R,
                             Args...>;
  using indirect_type = indirect<R,
                                 Args...>;
  using base_type = base<R,
                         Args...>;
  using align_type = std::conditional_t<(alignof(direct_type) > alignof(base_type)),
                                        indirect_type,
                                        direct_type>;
public:
  using type = std::conditional_t<(sizeof(direct_type) > sizeof(Storage)),
                                  indirect_type,
                                  align_type>;
};

template<typename Storage,
         typename T,
         typename Allocator,
         typename R,
         typename... Args>
using select_t = typename select<Storage,
                                 T,
                                 Allocator,
                                 R,
                                 Args...>::type;

}

#ifdef ASIO_URING_DOXYGEN_RUNNING
/**
 *  Provides type erased storage for a callable
 *  object with a certain signature.
 *
 *  As compared to `std::function` makes the following
 *  trade offs:
 *
 *  - Configurable small function optimization
 *  - Movable but not copyable (and therefore stored
 *    function objects need not be copyable)
 *  - Allocation is customizable (`std::function`
 *    `Allocator` support was deprecated and removed)
 *    and customized allocation meets the requirements
 *    of Boost.Asio and the Networking TS (i.e. if an
 *    allocator is used to allocate storage for a
 *    callable object that storage is deallocated before
 *    the callable object is invoked)
 *  - Not assignable
 *  - Stored objects may only be invoked once
 *
 *  \tparam Buffer
 *    The size of the buffer to use for the small function
 *    optimization. Note that for very small values a
 *    larger value may be chosen internally.
 *  \tparam Signature
 *    A function signature with which stored callable
 *    objects shall be invocable. Defaults to `void()`.
 */
template<std::size_t Buffer,
         typename Signature>
class callable_storage {
  callable_storage() = delete;
  callable_storage(const callable_storage&) = delete;
  callable_storage(callable_storage&&) = delete;
  callable_storage& operator=(const callable_storage&) = delete;
  callable_storage& operator=(callable_storage&&) = delete;
  /**
   *  Creates a callable_storage by storing some
   *  callable object.
   *
   *  \tparam T
   *    The type of object to store.
   *  \tparam Allocator
   *    The type of `Allocator` to use to customize
   *    the allocation strategy.
   *
   *  \param [in] t
   *    The object to store.
   *  \param [in] alloc
   *    The `Allocator` to use to allocate memory if
   *    necessary. If `t` is sufficiently small that
   *    it qualifies for the small function optimization
   *    this argument is not used.
   */
  template<typename T,
           typename Allocator>
  callable_storage(T&& t,
                   const Allocator& alloc);
  /**
   *  Invokes the stored object.
   *
   *  Invoking this function multiple times on the same
   *  object results in undefined behavior.
   *
   *  \param [in] args
   *    The arguments to the stored object.
   *
   *  \return
   *    The result of invoking the stored object.
   */
  R operator()(Args... args);
};

#else

template<std::size_t,
         typename Signature = void()>
class callable_storage;

template<std::size_t Buffer,
         typename R,
         typename... Args>
class callable_storage<Buffer,
                       R(Args...)>
{
private:
  using base_type = detail::callable_storage::base<R,
                                                   Args...>;
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
                                                                 Allocator,
                                                                 R,
                                                                 Args...>(std::forward<T>(t),
                                                                          alloc);
#ifdef NDEBUG
    (void)ptr;
#else
    ptr_ = ptr;
#endif
  }
  ~callable_storage() noexcept {
    get().~base_type();
  }
  R operator()(Args... args) {
    return get().invoke(std::forward<Args>(args)...);
  }
private:
  base_type& get() noexcept {
    auto ptr = reinterpret_cast<base_type*>(&storage_);
    assert(ptr_);
    assert(ptr == ptr_);
    return *ptr;
  }
  const base_type& get() const noexcept {
    auto ptr = reinterpret_cast<base_type*>(&storage_);
    assert(ptr_);
    assert(ptr == ptr_);
    return *ptr;
  }
  using storage_type = std::aligned_union_t<Buffer,
                                            detail::callable_storage::indirect<R,
                                                                               Args...>>;
  storage_type storage_;
#ifndef NDEBUG
  base_type*   ptr_;
#endif
};
#endif

}
