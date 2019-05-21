/**
 *  \file
 */

#pragma once

#include <optional>
#include <type_traits>
#include <utility>
#include <vector>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>
#include <boost/intrusive/options.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include "callable_storage.hpp"
#include "execution_context.hpp"
#include "liburing.hpp"
#include <sys/uio.h>

namespace asio_uring {

/**
 *  Models the `IoObjectService` concept from Boost.Asio
 *  and provides pools of:
 *
 *  - `::iovec` objects
 *  - Objects which derive from \ref execution_context::completion
 *    and which provide storage for completion handlers in
 *    accordance with the rules of Boost.Asio and the Networking
 *    TS
 *
 *  Note that while this object models `IoObjectService`
 *  it does not model `Service` as this would require
 *  relying on Boost.Asio-specific classes and functionality.
 */
class service {
private:
  using hook_type = boost::intrusive::list_member_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>>;
  using iovs_type = std::vector<::iovec>;
  class completion final : public execution_context::completion {
  friend class service;
  private:
    using function_type = callable_storage<256,
                                           void(::io_uring_cqe)>;
  public:
    explicit completion(service&);
    virtual void complete(const ::io_uring_cqe&) override;
    template<typename T,
             typename Allocator>
    void emplace(T&& t,
                 const Allocator& alloc) noexcept(std::is_nothrow_constructible_v<function_type,
                                                                                  T&&,
                                                                                  const Allocator&>)
    {
      assert(!wrapped_);
      wrapped_.emplace(std::forward<T>(t),
                       alloc);
      svc_.context().get_executor().on_work_started();
    }
    void reset() noexcept;
  private:
    service&                     svc_;
    hook_type                    service_;
    hook_type                    implementation_;
    std::optional<function_type> wrapped_;
    iovs_type                    iovs_;
  };
  template<hook_type(completion::*MemberPtr)>
  using list_t = boost::intrusive::list<completion,
                                        boost::intrusive::member_hook<completion,
                                                                      hook_type,
                                                                      MemberPtr>,
                                        boost::intrusive::constant_time_size<false>>;
  using implementation_list_type = list_t<&completion::implementation_>;
  class release_guard {
  public:
    release_guard(service&,
                  completion&) noexcept;
    release_guard(const release_guard&) = delete;
    release_guard(release_guard&&) = delete;
    release_guard& operator=(const release_guard&) = delete;
    release_guard& operator=(release_guard&&) = delete;
    ~release_guard() noexcept;
    void release() noexcept;
  private:
    service*    self_;
    completion& c_;
  };
public:
  /**
   *  A handle to the pool of \ref execution_context::completion
   *  objects a particular I/O object has acquired.
   */
  class implementation_type {
  friend class service;
  public:
    implementation_type() = default;
    implementation_type(const implementation_type&) = delete;
    implementation_type(implementation_type&&) = delete;
    implementation_type& operator=(const implementation_type&) = delete;
    implementation_type& operator=(implementation_type&&) = delete;
  private:
    using list_type = implementation_list_type;
    class to_const_void_pointer {
    public:
      using result_type = const void*;
      using argument_type = const completion&;
      result_type operator()(argument_type arg) const noexcept;
    };
    list_type list_;
  public:
    /**
     *  @{
     *  A type which models `ForwardIterator` and whose
     *  `value_type` is `const void*`. 
     */
    using const_iterator = boost::transform_iterator<to_const_void_pointer,
                                                     list_type::const_iterator>;
    using iterator = const_iterator;
    /**
     *  @}
     *  Obtains an iterator to the first `user_data` value
     *  in use by this handle.
     *
     *  \return
     *    An \ref const_iterator "iterator".
     */
    const_iterator begin() const noexcept;
    /**
     *  Obtains an iterator to one past the last `user_data`
     *  value in use by this handle.
     *
     *  \return
     *    An \ref const_iterator "iterator".
     */
    const_iterator end() const noexcept;
  };
  /**
   *  Creates a service which is associated with a certain
   *  \ref execution_context.
   *
   *  \param [in] ctx
   *    The \ref execution_context. This reference must remain
   *    valid for the lifetime of the newly-created object or
   *    the behavior is undefined.
   */
  explicit service(execution_context& ctx);
  service(const service&) = delete;
  service(service&&) = delete;
  service& operator=(const service&) = delete;
  service& operator=(service&&) = delete;
#ifndef ASIO_URING_DOXYGEN_RUNNING
  ~service() noexcept;
#endif
  /**
   *  Destroys all stored completion handlers.
   *
   *  Note that this does not cause all outstanding operations
   *  to complete, it merely causes them to do nothing when
   *  they complete.
   */
  void shutdown() noexcept;
  /**
   *  Retrieves the associated \ref execution_context.
   *
   *  A reference to the associated \ref execution_context.
   */
  execution_context& context() const noexcept;
  /**
   *  Initializes a \ref implementation_type "handle".
   *
   *  \param [in, out] impl
   *    The \ref implementation_type "handle" to initialize.
   */
  void construct(implementation_type& impl);
  /**
   *  Deinitializes a \ref implementation_type "handle".
   *
   *  Note that this merely cleans up the
   *  \ref implementation_type handle. All outstanding
   *  operations initiated against that handle are
   *  still outstanding and may complete in the future.
   *
   *  \param [in] impl
   *    The \ref implementation_type "handle" to deinitialize.
   */
  void destroy(implementation_type& impl) noexcept;
  /**
   *  Initializes a \ref implementation_type "handle" by
   *  transferring another \ref implementation_type "handle".
   *
   *  \param [in, out] impl
   *    The \ref implementation_type "handle" to initialize.
   *  \param [in] src
   *    The \ref implementation_type "handle" from which to
   *    transfer.
   */
  void move_construct(implementation_type& impl,
                      implementation_type& src) noexcept;
  /**
   *  Deinitializes a \ref implementation_type "handle" and
   *  then initializes it by transferring another
   *  \ref implementation_type "handle".
   *
   *  \param [in, out] impl
   *    The \ref implementation_type "handle" to initialize.
   *  \param [in] svc
   *    The service associated with `src`. If this is not
   *    the same as this object the behavior is undefined.
   *  \param [in] src
   *    The \ref implementation_type "handle" from which to
   *    transfer.
   */
  void move_assign(implementation_type& impl,
                   service& svc,
                   implementation_type& src) noexcept;
  /**
   *  Initiates an operation against the `io_uring`.
   *
   *  \tparam Function
   *    A callable object which is invocable with
   *    the following signature:
   *    \code
   *    void(::io_uring_sqe&,
   *         void*) noexcept;
   *    \endcode
   *    Where the first argument is the submission
   *    queue entry (which this function is expected
   *    to initialize) and the second argument is
   *    a pointer to \ref execution_context::completion
   *    cast to `void*`. Note that the second argument
   *    is for reference only: After the return from
   *    the callable object `::io_uring_sqe::user_data`
   *    will be populated with this value whether the
   *    caller does the population or not.
   *  \tparam T
   *    The completion handler which is invocable
   *    with the following signature:
   *    \code
   *    void(const ::io_uring_cqe&);
   *    \endcode
   *    Note that none of the guarantees of Boost.Asio
   *    or the Networking TS regarding executors or
   *    allocators is honored for this completion
   *    handler: It is always invoked in the
   *    \ref execution_context associated with this
   *    object and storage is always allocated with
   *    `alloc`. It is the responsibility of the caller
   *    of this function to honor the guarantees reganding
   *    executor and allocator associations.
   *  \tparam Allocator
   *    The type of allocator to use to allocate storage
   *    for the completion handler (if necessary).
   *
   *  \param [in, out] impl
   *    The \ref implementation_type "handle" to associate
   *    the operation with.
   *  \param [in] f
   *    The function to use to initialize the submission
   *    queue entry.
   *  \param [in] t
   *    The completion handler.
   *  \param [in] alloc
   *    The `Allocator`.
   */
  template<typename Function,
           typename T,
           typename Allocator>
  void initiate(implementation_type& impl,
                Function f,
                T&& t,
                const Allocator& alloc)
  {
    auto&& c = acquire(impl);
    release_guard g(*this,
                    c);
    c.emplace(std::forward<T>(t),
              alloc);
    auto&& sqe = ctx_.get_sqe();
    void* user_data = &c;
    static_assert(noexcept(f(sqe,
                             user_data)));
    f(sqe,
      user_data);
    submit(sqe,
           c);
    g.release();
  }
  /**
   *  Initiates an operation against the `io_uring`.
   *
   *  \tparam Function
   *    A callable object which is invocable with
   *    the following signature:
   *    \code
   *    void(::io_uring_sqe&,
   *         ::iovec*,
   *         void*) noexcept;
   *    \endcode
   *    Where the arguments are as follows:
   *    1. The submission queue entry (which this function
   *       is expected to initialize)
   *    2. A pointer to `iovs` `::iovec` objects
   *    3. A pointer to \ref execution_context::completion
   *       cast to `void*` (for reference only there is no
   *       need to populate this in the submission queue entry)
   *  \tparam T
   *    The completion handler which is invocable
   *    with the following signature:
   *    \code
   *    void(const ::io_uring_cqe&);
   *    \endcode
   *    Note that none of the guarantees of Boost.Asio
   *    or the Networking TS regarding executors or
   *    allocators is honored for this completion
   *    handler: It is always invoked in the
   *    \ref execution_context associated with this
   *    object and storage is always allocated with
   *    `alloc`. It is the responsibility of the caller
   *    of this function to honor the guarantees reganding
   *    executor and allocator associations.
   *  \tparam Allocator
   *    The type of allocator to use to allocate storage
   *    for the completion handler (if necessary).
   *
   *  \param [in, out] impl
   *    The \ref implementation_type "handle" to associate
   *    the operation with.
   *  \param [in] iovs
   *    The number of `::iovec` objects to main available
   *    from the managed pool via the second argument to
   *    `f`.
   *  \param [in] f
   *    The function to use to initialize the submission
   *    queue entry.
   *  \param [in] t
   *    The completion handler.
   *  \param [in] alloc
   *    The `Allocator`.
   */
  template<typename Function,
           typename T,
           typename Allocator>
  void initiate(implementation_type& impl,
                std::size_t iovs,
                Function f,
                T&& t,
                const Allocator& alloc)
  {
    auto&& c = acquire(impl);
    release_guard g(*this,
                    c);
    c.iovs_ = acquire(iovs);
    c.emplace(std::forward<T>(t),
              alloc);
    auto&& sqe = ctx_.get_sqe();
    void* user_data = &c;
    static_assert(noexcept(f(sqe,
                             c.iovs_.data(),
                             user_data)));
    f(sqe,
      c.iovs_.data(),
      user_data);
    submit(sqe,
           c);
    g.release();
  }
private:
  completion& maybe_allocate();
  completion& acquire(implementation_type&);
  iovs_type acquire(iovs_type::size_type);
  void release(completion&) noexcept;
  void release(iovs_type&) noexcept;
  void submit(::io_uring_sqe&,
              completion&);
  using list_type = list_t<&completion::service_>;
  using iovs_cache_type = std::vector<iovs_type>;
  void destroy_list(list_type&) noexcept;
  execution_context& ctx_;
  list_type          free_;
  list_type          in_use_;
  iovs_cache_type    iovs_cache_;
};

}
