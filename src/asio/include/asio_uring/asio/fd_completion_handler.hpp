/**
 *  \file
 */

#pragma once

#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>
#include <asio_uring/fd.hpp>
#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/associated_executor.hpp>

namespace asio_uring::asio {

template<typename CompletionHandler>
class fd_completion_handler {
public:
  using completion_handler_type = CompletionHandler;
  using fd_type = std::shared_ptr<fd>;
  fd_completion_handler() = delete;
  fd_completion_handler(const fd_completion_handler&) = delete;
  fd_completion_handler(fd_completion_handler&&) = default;
  fd_completion_handler& operator=(const fd_completion_handler&) = delete;
  fd_completion_handler& operator=(fd_completion_handler&&) = default;
  fd_completion_handler(CompletionHandler h,
                        fd_type fd) noexcept(std::is_nothrow_move_constructible_v<CompletionHandler>)
    : h_ (std::move(h)),
      fd_(std::move(fd))
  {
    assert(fd_);
  }
  template<typename... Args>
  void operator()(Args&&... args) noexcept(noexcept(std::declval<CompletionHandler&>()(std::forward<Args>(std::declval<Args>())...))) {
    assert(fd_);
    fd_type fd;
    using std::swap;
    swap(fd,
         fd_);
    h_(std::forward<Args>(args)...);
  }
  completion_handler_type& completion_handler() noexcept {
    return h_;
  }
  const completion_handler_type& completion_handler() const noexcept {
    return h_;
  }
private:
  CompletionHandler h_;
  fd_type           fd_;
};

}

namespace boost::asio {

template<typename CompletionHandler,
         typename Allocator>
class associated_allocator<asio_uring::asio::fd_completion_handler<CompletionHandler>,
                           Allocator>
{
public:
  using type = typename associated_allocator<CompletionHandler,
                                             Allocator>::type;
  static type get(const asio_uring::asio::fd_completion_handler<CompletionHandler>& h,
                  const Allocator& alloc = Allocator()) noexcept
  {
    return asio::get_associated_allocator(h.completion_handler(),
                                          alloc);
  }
};

template<typename CompletionHandler,
         typename Executor>
class associated_executor<asio_uring::asio::fd_completion_handler<CompletionHandler>,
                          Executor>
{
public:
  using type = typename associated_executor<CompletionHandler,
                                            Executor>::type;
  static type get(const asio_uring::asio::fd_completion_handler<CompletionHandler>& h,
                  const Executor& ex = Executor()) noexcept
  {
    return asio::get_associated_executor(h.completion_handler(),
                                         ex);
  }
};

}
