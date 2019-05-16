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

class execution_context {
public:
  class completion {
  public:
    completion() = default;
    virtual void complete(const ::io_uring_cqe& cqe) = 0;
  };
  using count_type = std::size_t;
  using native_handle_type = uring::native_handle_type;
  using const_native_handle_type = uring::const_native_handle_type;
  class executor_type {
  public:
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
  };
  explicit execution_context(unsigned entries,
                             unsigned flags = 0);
  execution_context(const execution_context&) = delete;
  execution_context(execution_context&&) = delete;
  execution_context& operator=(const execution_context&) = delete;
  execution_context& operator=(execution_context&&) = delete;
  executor_type get_executor() noexcept;
  void restart();
  count_type run();
  count_type run_one();
  count_type poll();
  count_type poll_one();
  void stop() noexcept;
  native_handle_type native_handle() noexcept;
  const_native_handle_type native_handle() const noexcept;
  bool running_in_this_thread() const noexcept;
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
               const bool&);
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
