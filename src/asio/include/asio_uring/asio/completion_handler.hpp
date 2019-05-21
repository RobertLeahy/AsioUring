/**
 *  \file
 */

#pragma once

#include <functional>
#include <memory>
#include <utility>
#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include "execution_context.hpp"

namespace asio_uring::asio {

/**
 *  A wrapper for completion handlers which
 *  maintains the guarantees of Boost.Asio
 *  regarding allocator and executor
 *  associations. Notably:
 *
 *  - Holds a `boost::asio::executor_work_guard` on
 *    the associated `Executor` ensuring it does not
 *    run out of work while the completion handler is
 *    pending
 *  - Ensures that the completion handler is invoked
 *    on the associated `Executor`
 *  - Ensures that all intermediate storage is allocated
 *    using the associated `Allocator`
 *
 *  \tparam CompletionHandler
 *    The completion handler type to wrap.
 *  \tparam Allocator
 *    The fallback allocator type. Defaults to
 *    `std::allocator<void>`.
 */
template<typename CompletionHandler,
         typename Allocator = std::allocator<void>>
class completion_handler {
private:
  using associated_allocator = typename boost::asio::associated_allocator<CompletionHandler,
                                                                          Allocator>;
  using associated_executor = typename boost::asio::associated_executor<CompletionHandler,
                                                                        execution_context::executor_type>;
public:
  /**
   *  A type alias for the type of `Allocator` associated
   *  with `CompletionHandler` given the fallback `Allocator`
   *  type on which this class template was instantiated.
   */
  using allocator_type = typename associated_allocator::type;
  /**
   *  A type alias for the type of `Executor` associated
   *  with `CompletionHandler` given a fallback `Executor`
   *  type of \ref asio_uring::execution_context::executor_type "execution_context::executor_type".
   */
  using executor_type = typename associated_executor::type;
  completion_handler() = delete;
  completion_handler(const completion_handler&) = delete;
  completion_handler(completion_handler&&) = default;
  completion_handler& operator=(const completion_handler&) = delete;
  completion_handler& operator=(completion_handler&&) = default;
  /**
   *  Wraps a completion handler.
   *
   *  \param [in] h
   *    The completion handler to wrap.
   *  \param [in] alloc
   *    An instance of the fallback `Allocator`.
   *  \param [in] ex
   *    An instance of the fallback `Executor`.
   */
  completion_handler(CompletionHandler h,
                     const Allocator& alloc,
                     const execution_context::executor_type& ex)
    : alloc_(boost::asio::get_associated_allocator(h,
                                                   alloc)),
      work_ (boost::asio::get_associated_executor(h,
                                                  ex)),
      h_    (std::move(h))
  {}
  /**
   *  Wraps a completion handler.
   *
   *  The fallback `Allocator` is defaulted to
   *  `Allocator()`.
   *
   *  \param [in] h
   *    The completion handler to wrap.
   *  \param [in] ex
   *    An instance of the fallback `Executor`.
   */
  completion_handler(CompletionHandler h,
                     const execution_context::executor_type& ex)
    : completion_handler(std::move(h),
                         Allocator(),
                         ex)
  {}
  /**
   *  Retrieves the associated `Allocator`.
   *
   *  \return
   *    A copy of the associated `Allocator`.
   */
  allocator_type get_allocator() const noexcept {
    return alloc_;
  }
  /**
   *  Retrieves the associated `Executor`.
   *
   *  \return
   *    A copy of the associated `Executor`.
   */
  executor_type get_executor() const noexcept {
    return work_.get_executor();
  }
  /**
   *  Invokes the wrapped completion handler `h` with
   *  arguments `args` using the `ExecutionContext`
   *  to which the `Executor` returned by
   *  \ref get_executor submits work with all
   *  intermediate allocations performed by the
   *  `Allocator` of which \ref get_allocator returns
   *  a copy.
   *
   *  Note that since:
   *
   *  - The fallback `Executor` is
   *    \ref asio_uring::execution_context::executor_type "execution_context::executor_type"
   *  - \ref asio_uring::execution_context invokes work
   *    submitted with `dispatch` synchronously
   *  - This function uses `dispatch` to submit the
   *    completion handler
   *
   *  For all completion handlers which do not customize
   *  `Executor` associations the completion handler will
   *  be invoked immediately at little-to-no cost when
   *  completing an `io_uring` asynchronous operation (since
   *  the caller will be on a thread which is running in
   *  the execution context).
   *
   *  \tparam Args
   *    The types of arguments to forward to the
   *    completion handler.
   *
   *  \param [in] args
   *    The arguments to forward through to the
   *    invocation.
   */
  template<typename... Args>
  void operator()(Args&&... args) {
    auto g = std::move(work_);
    g.get_executor().dispatch(std::bind(std::move(h_),
                                        std::forward<Args>(args)...),
                              alloc_);
  }
private:
  using work_guard_type = boost::asio::executor_work_guard<executor_type>;
  allocator_type                   alloc_;
  work_guard_type                  work_;
  CompletionHandler                h_;
};

#ifndef ASIO_URING_DOXYGEN_RUNNING
template<typename CompletionHandler>
completion_handler(CompletionHandler,
                   const execution_context::executor_type&) -> completion_handler<CompletionHandler,
                                                                                  std::allocator<void>>;
#endif

}
