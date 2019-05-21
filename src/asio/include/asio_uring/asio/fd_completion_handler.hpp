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

/**
 *  A completion handler wrapper which preserves
 *  `Executor` and `Allocator` associations as
 *  per Boost.Asio and also contains a
 *  `std::shared_ptr<fd>` such that the owned
 *  file descriptor's lifetime is extended at
 *  least until the completion handler's invocation
 *  completes.
 *
 *  \sa
 *    fd_completion_token
 */
template<typename CompletionHandler>
class fd_completion_handler {
public:
#ifndef ASIO_URING_DOXYGEN_RUNNING
  using completion_handler_type = CompletionHandler;
#endif
  /**
   *  A type alias for `std::shared_ptr<fd>`.
   */
  using fd_type = std::shared_ptr<fd>;
  fd_completion_handler() = delete;
  fd_completion_handler(const fd_completion_handler&) = delete;
  fd_completion_handler(fd_completion_handler&&) = default;
  fd_completion_handler& operator=(const fd_completion_handler&) = delete;
  fd_completion_handler& operator=(fd_completion_handler&&) = default;
  /**
   *  Wraps a completion handler and a \ref fd "file descriptor".
   *
   *  \param [in] h
   *    The completion handler.
   *  \param [in] fd
   *    The `std::shared_ptr` to the \ref fd "file descripctor".
   *    If `!fd` is `true` the behavior is undefined.
   */
  fd_completion_handler(CompletionHandler h,
                        fd_type fd) noexcept(std::is_nothrow_move_constructible_v<CompletionHandler>)
    : h_ (std::move(h)),
      fd_(std::move(fd))
  {
    assert(fd_);
  }
  /**
   *  Invokes the underlying completion handler and
   *  then (regardless of whether that invocation exits
   *  via exception or not) releases the reference count
   *  which may allow the lifetime of the \ref fd "file descriptor"
   *  to end.
   *
   *  Note that once this function is invoked (whether it
   *  ends in exception or not) invoking it again on the
   *  same object is undefined behavior (this follows
   *  logically from the fact that completion handlers
   *  themselves may only be invoked a single time).
   *
   *  \tparam Args
   *    The types of arguments to forward through to the
   *    completion handler.
   *
   *  \param [in] args
   *    The arguments to forward through to the completion
   *    handler.
   */
  template<typename... Args>
  void operator()(Args&&... args) noexcept(noexcept(std::declval<CompletionHandler&>()(std::forward<Args>(std::declval<Args>())...))) {
    assert(fd_);
    fd_type fd;
    using std::swap;
    swap(fd,
         fd_);
    h_(std::forward<Args>(args)...);
  }
#ifndef ASIO_URING_DOXYGEN_RUNNING
  completion_handler_type& completion_handler() noexcept {
    return h_;
  }
  const completion_handler_type& completion_handler() const noexcept {
    return h_;
  }
#endif
private:
  CompletionHandler h_;
  fd_type           fd_;
};

}

#ifndef ASIO_URING_DOXYGEN_RUNNING
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
#endif
