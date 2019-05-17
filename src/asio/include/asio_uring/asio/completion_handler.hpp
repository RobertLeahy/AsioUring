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

template<typename CompletionHandler,
         typename Allocator = std::allocator<void>>
class completion_handler {
private:
  using associated_allocator = typename boost::asio::associated_allocator<CompletionHandler,
                                                                          Allocator>;
  using associated_executor = typename boost::asio::associated_executor<CompletionHandler,
                                                                        execution_context::executor_type>;
public:
  using allocator_type = typename associated_allocator::type;
  using executor_type = typename associated_executor::type;
  completion_handler() = delete;
  completion_handler(const completion_handler&) = delete;
  completion_handler(completion_handler&&) = default;
  completion_handler& operator=(const completion_handler&) = delete;
  completion_handler& operator=(completion_handler&&) = default;
  completion_handler(CompletionHandler h,
                     const Allocator& alloc,
                     const execution_context::executor_type& ex)
    : alloc_(boost::asio::get_associated_allocator(h,
                                                   alloc)),
      work_ (boost::asio::get_associated_executor(h,
                                                  ex)),
      h_    (std::move(h))
  {}
  completion_handler(CompletionHandler h,
                     const execution_context::executor_type& ex)
    : completion_handler(std::move(h),
                         Allocator(),
                         ex)
  {}
  allocator_type get_allocator() const noexcept {
    return alloc_;
  }
  executor_type get_executor() const noexcept {
    return work_.get_executor();
  }
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

template<typename CompletionHandler>
completion_handler(CompletionHandler,
                   const execution_context::executor_type&) -> completion_handler<CompletionHandler,
                                                                                  std::allocator<void>>;

}
