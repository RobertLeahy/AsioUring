/**
 *  \file
 */

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <optional>
#include <thread>
#include <type_traits>
#include <utility>
#include "callable_storage.hpp"
#include "eventfd.hpp"
#include "eventfd_queue.hpp"
#include "liburing.hpp"
#include "uring.hpp"

namespace asio_uring {

/**
 *  Models `ExecutionContext` on top of `io_uring`
 *  functionality.
 */
class execution_context {
public:
  /**
   *  A base class for all operations which use raw
   *  `io_uring` functionality.
   *
   *  The internal proactor of the execution context
   *  assumes that all `::io_uring_sqe::user_data` (and
   *  therefore all `::io_uring_cqe::user_data` by
   *  extension) are pointers to instances of this type.
   *  Accordingly upon dequeing a `::io_uring_cqe` that
   *  is not for an internal task the proactor immediately
   *  casts that value to this type and dispatches the
   *  completion.
   *
   *  Note that when using this base class to access
   *  raw `io_uring` functionality the proactor does not
   *  perform any memory management. It is up to the user
   *  to ensure the lifetime of their completion objects
   *  is as required to avoid undefined behavior and
   *  memory leaks.
   */
  class completion {
  public:
    completion() = default;
    virtual void complete(const ::io_uring_cqe& cqe) = 0;
  };
  /**
   *  The type used to represent a count of executed
   *  handlers.
   */
  using count_type = std::size_t;
  /**
   *  A type alias for \ref uring::native_handle_type.
   */
  using native_handle_type = uring::native_handle_type;
  /**
   *  A type alias for \ref uring::const_native_handle_type.
   */
  using const_native_handle_type = uring::const_native_handle_type;
  /**
   *  A type which models `Executor`.
   */
  class executor_type {
  public:
#ifndef ASIO_URING_DOXYGEN_RUNNING
    explicit executor_type(execution_context&) noexcept;
    executor_type() = delete;
    executor_type(const executor_type&) = default;
    executor_type(executor_type&&) = default;
    executor_type& operator=(const executor_type&) = default;
    executor_type& operator=(executor_type&&) = default;
    execution_context& context() const noexcept;
    void on_work_started() const noexcept;
    void on_work_finished() const noexcept;
    bool operator==(const executor_type& rhs) const noexcept;
    bool operator!=(const executor_type& rhs) const noexcept;
    template<typename NullaryFunction,
             typename Allocator>
    void dispatch(NullaryFunction&& function,
                  const Allocator& alloc) const
    {
      assert(ctx_);
      if (ctx_->running_in_this_thread()) {
        std::decay_t<NullaryFunction> local(std::forward<NullaryFunction>(function));
        local();
        return;
      }
      post(std::forward<NullaryFunction>(function),
           alloc);
    }
    template<typename NullaryFunction,
             typename Allocator>
    void defer(NullaryFunction&& function,
               const Allocator& alloc) const
    {
      post(std::forward<NullaryFunction>(function),
           alloc);
    }
    template<typename NullaryFunction,
             typename Allocator>
    void post(NullaryFunction&& function,
              const Allocator& alloc) const
    {
      assert(ctx_);
      on_work_started();
      try {
        ctx_->q_.emplace(std::forward<NullaryFunction>(function),
                         alloc);
      } catch (...) {
        on_work_finished();
      }
    }
  private:
    execution_context* ctx_;
#endif
  };
  /**
   *  Creates an execution_context by passing arguments
   *  through to a constructor of \ref uring.
   *
   *  \param [in] entries
   *    See documentation for \ref uring::uring(unsigned,unsigned) "the corresponding constructor of uring".
   *  \param [in] flags
   *    See documentation for \ref uring::uring(unsigned,unsigned) "the corresponding constructor of uring".
   */
  explicit execution_context(unsigned entries,
                             unsigned flags = 0);
  execution_context(const execution_context&) = delete;
  execution_context(execution_context&&) = delete;
  execution_context& operator=(const execution_context&) = delete;
  execution_context& operator=(execution_context&&) = delete;
  /**
   *  Retrieves an \ref executor_type "executor" which
   *  submits work to this execution context.
   *
   *  \return
   *    An \ref executor_type "executor".
   */
  executor_type get_executor() noexcept;
  /**
   *  Prepares the execution context for a subsequent
   *  call to \ref run, \ref run_one, \ref poll, or
   *  \ref poll_one.
   *
   *  Idempotent.
   */
  void restart();
  /**
   *  Runs handlers until the execution context runs
   *  out of work.
   *
   *  Note if this invocation ends due to an exception
   *  from a handler it is safe to call again without
   *  a call to \ref restart. If however the exception
   *  was from the implementation of the execution context
   *  itself a call to \ref restart is required.
   *
   *  \warning
   *    This function is not thread safe: Multiple threads
   *    may not call \ref run, \ref run_one, \ref poll, or
   *    \ref poll_one simultaneously.
   *
   *  \return
   *    The number of handlers run.
   */
  count_type run();
  /**
   *  Runs handlers until either:
   *
   *  - One handler is run
   *  - The execution handler runs out of work
   *
   *  Note if this invocation ends due to an exception
   *  from a handler it is safe to call again without
   *  a call to \ref restart. If however the exception
   *  was from the implementation of the execution context
   *  itself a call to \ref restart is required.
   *
   *  \warning
   *    This function is not thread safe: Multiple threads
   *    may not call \ref run, \ref run_one, \ref poll, or
   *    \ref poll_one simultaneously.
   *
   *  \return
   *    The number of handlers run.
   */
  count_type run_one();
  /**
   *  Runs handlers until either:
   *
   *  - The execution handler runs out of work
   *  - No work is available to run immediately
   *
   *  Note if this invocation ends due to an exception
   *  from a handler it is safe to call again without
   *  a call to \ref restart. If however the exception
   *  was from the implementation of the execution context
   *  itself a call to \ref restart is required.
   *
   *  \warning
   *    This function is not thread safe: Multiple threads
   *    may not call \ref run, \ref run_one, \ref poll, or
   *    \ref poll_one simultaneously.
   *
   *  \return
   *    The number of handlers run.
   */
  count_type poll();
  /**
   *  Runs handlers until either:
   *
   *  - One handler is run
   *  - The execution handler runs out of work
   *  - No work is available to run immediately
   *
   *  Note if this invocation ends due to an exception
   *  from a handler it is safe to call again without
   *  a call to \ref restart. If however the exception
   *  was from the implementation of the execution context
   *  itself a call to \ref restart is required.
   *
   *  \warning
   *    This function is not thread safe: Multiple threads
   *    may not call \ref run, \ref run_one, \ref poll, or
   *    \ref poll_one simultaneously.
   *
   *  \return
   *    The number of handlers run.
   */
  count_type poll_one();
  /**
   *  Causes pending invocations of \ref run or \ref run_one
   *  to return regardless of whether they've invoked any
   *  handlers or whether the execution context is out
   *  of work.
   */
  void stop() noexcept;
  /**
   *  @{
   *  Invokes \ref uring::native_handle.
   *
   *  \return
   *    A handle to the `io_uring`.
   */
  native_handle_type native_handle() noexcept;
  const_native_handle_type native_handle() const noexcept;
  /**
   *  @}
   *  Determines whether the execution context is
   *  currently running in this thread.
   *
   *  If this function returns `true` then dispatching
   *  (as opposed to defering or posting) handlers
   *  against an \ref executor_type "executor" associated
   *  with this execution context will invoke that
   *  callable object in a blocking manner (i.e. the
   *  call stack will grow down as the callable object
   *  is invoked below the caller on the call stack
   *  rather than being enqueued for asynchronous
   *  invocation).
   *
   *  \return
   *    `true` if the execution context is running in this
   *    thread, `false` otherwise.
   */
  bool running_in_this_thread() const noexcept;
  /**
   *  Attempts to obtain a submission queue entry
   *  from the ring by calling `::io_uring_get_sqe`
   *  and checking the result for `nullptr`.
   *
   *  Throws if a submission queue entry could not
   *  be obtained (i.e. if `::io_uring_get_sqe` returns
   *  `nullptr`).
   *
   *  \return
   *    A reference to an `::io_uring_sqe`.
   */
  ::io_uring_sqe& get_sqe();
private:
  bool out_of_work() const noexcept;
  class tid_guard {
  public:
    tid_guard() = delete;
    tid_guard(const tid_guard&) = delete;
    tid_guard(tid_guard&&) = delete;
    tid_guard& operator=(const tid_guard&) = delete;
    tid_guard& operator=(tid_guard&&) = delete;
    explicit tid_guard(std::atomic<std::thread::id>&) noexcept;
    ~tid_guard() noexcept;
  private:
    std::atomic<std::thread::id>& tid_;
  };
  class handle_cqe_type {
  public:
    handle_cqe_type() noexcept;
    count_type handlers;
    bool       stopped;
  };
  template<bool>
  count_type all_impl();
  template<bool>
  count_type one_impl();
  template<bool>
  handle_cqe_type impl();
  bool stopped() const noexcept;
  void restart(std::size_t,
               bool&);
  void restart_if(std::size_t,
                  bool&);
  handle_cqe_type handle_cqe(::io_uring_cqe&);
  using function_type = callable_storage<256>;
  using queue_type = eventfd_queue<function_type>;
  bool service_queue(queue_type::integer_type);
  bool service_queue();
  queue_type                   q_;
  bool                         q_started_;
  queue_type::integer_type     pending_;
  eventfd                      stop_;
  bool                         stop_started_;
  eventfd                      zero_;
  bool                         zero_started_;
  std::atomic<std::size_t>     work_;
  std::atomic<bool>            stopped_;
  uring                        u_;
  std::atomic<std::thread::id> tid_;
};

}
