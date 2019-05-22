/**
 *  \file
 */

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <system_error>
#include <type_traits>
#include <utility>
#include <asio_uring/fd.hpp>
#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/post.hpp>
#include "error_code.hpp"
#include "execution_context.hpp"
#include "file_object.hpp"
#include "read.hpp"
#include "write.hpp"
#include <sys/poll.h>

namespace asio_uring::asio {

namespace detail {

template<typename CompletionHandler,
         typename Invoker>
class poll_file_op {
public:
  poll_file_op(Invoker i,
               CompletionHandler h) noexcept(std::is_nothrow_move_constructible_v<CompletionHandler> &&
                                             std::is_nothrow_move_constructible_v<Invoker>)
    : i_(std::move(i)),
      h_(std::move(h))
  {}
  void operator()(std::error_code ec) {
    i_(ec,
       std::move(h_));
  }
  const CompletionHandler& completion_handler() const noexcept {
    return h_;
  }
private:
  Invoker           i_;
  CompletionHandler h_;
};

}

/**
 *  An I/O object for asynchronous `io_uring` operations
 *  against file descriptors which are not suitable
 *  for use with \ref async_file.
 *
 *  This class models:
 *
 *  - `AsyncReadStream`
 *  - `AsyncWriteStream`
 *
 *  \sa
 *    async_file
 */
class poll_file : public file_object {
private:
  using signature = void(std::error_code,
                         std::size_t);
public:
#ifndef ASIO_URING_DOXYGEN_RUNNING
  poll_file(execution_context& ctx,
            fd file);
#endif
  /**
   *  Initiates an asynchronous operation which
   *  completes when the owned file descriptor
   *  becomes readable (i.e. when `poll` would
   *  return with `POLLIN`).
   *
   *  \tparam CompletionToken
   *    A completion token whose associated
   *    completion handler in invocable with the
   *    following signature:
   *    \code
   *    void(std::error_code);
   *    \endcode
   *    Where the argument is the result of the
   *    operation.
   *
   *  \param [in] token
   *    The completion token to use to notify the
   *    caller of completion.
   *
   *  \return
   *    Whatever is appropriate given `CompletionToken`
   *    and `token`.
   */
  template<typename CompletionToken>
  auto async_poll_in(CompletionToken&& token) {
    return get_service().initiate_poll_add(get_implementation(),
                                           native_handle(),
                                           POLLIN,
                                           wrap_token(std::forward<CompletionToken>(token)));
  }
  /**
   *  Initiates an asynchronous operation which
   *  completes when the owned file descriptor
   *  becomes writable (i.e. when `poll` would
   *  return with `POLLOUT`).
   *
   *  \tparam CompletionToken
   *    A completion token whose associated
   *    completion handler in invocable with the
   *    following signature:
   *    \code
   *    void(std::error_code);
   *    \endcode
   *    Where the argument is the result of the
   *    operation.
   *
   *  \param [in] token
   *    The completion token to use to notify the
   *    caller of completion.
   *
   *  \return
   *    Whatever is appropriate given `CompletionToken`
   *    and `token`.
   */
  template<typename CompletionToken>
  auto async_poll_out(CompletionToken&& token) {
    return get_service().initiate_poll_add(get_implementation(),
                                           native_handle(),
                                           POLLOUT,
                                           wrap_token(std::forward<CompletionToken>(token)));
  }
protected:
  /**
   *  Submits the completion handler associated with
   *  a certain completion token for execution on
   *  either its associated `Executor` (if it has such
   *  an association) or the `Executor` obtained by
   *  calling \ref basic_io_object::get_executor "get_executor"
   *  (otherwise) using `post` and synthesizing a return
   *  value using `boost::asio::async_result`.
   *
   *  \tparam CompletionToken
   *    The completion token to use to deduce the
   *    completion handler and returned value.
   *  \tparam Args
   *    Arguments to pass through to the deduced
   *    completion handler.
   *
   *  \param [in] token
   *    The completion token.
   *  \param [in] args
   *    The arguments to provide to the deduced completion
   *    handler.
   *
   *  \return
   *    Whatever is appropriate given `CompletionToken`
   *    and `token`.
   */
  template<typename CompletionToken,
           typename... Args>
  auto post(CompletionToken token,
            Args&&... args)
  {
    using signature_type = void(std::decay_t<Args>...);
    using async_result_type = boost::asio::async_result<std::decay_t<CompletionToken>,
                                                        signature_type>;
    using completion_handler_type = typename async_result_type::completion_handler_type;
    completion_handler_type h(std::forward<CompletionToken>(token));
    auto ex = boost::asio::get_associated_executor(h,
                                                   get_executor());
    auto alloc = boost::asio::get_associated_allocator(h,
                                                       std::allocator<void>());
    ex.post(std::bind(std::move(h),
                      std::forward<Args>(args)...),
            alloc);
  }
  /**
   *  Asynchronously waits for the underlying file descriptor
   *  to become ready for reading or writing and then
   *  allows the completion handler to be invoked after a
   *  synchronous operation takes place.
   *
   *  This simplifies the implementation of asynchronous
   *  operations which perform the typical POSIX
   *  I/O process of:
   *
   *  1. Wait for readiness (`poll`, `select`, etc.)
   *  2. Perform synchronous, non-blocking operation
   *     (`read`, `write`, etc.)
   *
   *  By allowing the caller to defer Boost.Asio boilerplate
   *  and the asynchronous wait for readiness to this
   *  initiating function.
   *
   *  \tparam In
   *    `true` for `POLLIN`, `false` for `POLLOUT`.
   *  \tparam Signature
   *    The signature of the completion handler for the
   *    operation being initiated.
   *  \tparam Invoker
   *    The type of function which will be invoked to perform
   *    the synchronous part of the operation. Must be invocable
   *    with a signature of:
   *    \code
   *    template<typename CompletionHandler>
   *    void(std::error_code,
   *         CompletionHandler);
   *    \endcode
   *    Where the first argument is the result of the asynchronous
   *    wait for readiness and the second argument is the final
   *    completion handler.
   *  \tparam CompletionToken
   *    A completion token type.
   *
   *  \param [in] i
   *    The function to invoke to perform the synchronous part
   *    of the operation.
   *  \param [in] token
   *    The completion token to use to notify the caller of
   *    completion.
   *
   *  \return
   *    Whatever is appropriate given `CompletionToken` and
   *    `token`.
   */
  template<bool In,
           typename Signature,
           typename Invoker,
           typename CompletionToken>
  auto async_poll_then(Invoker i,
                       CompletionToken&& token)
  {
    using async_result_type = boost::asio::async_result<std::decay_t<CompletionToken>,
                                                        Signature>;
    using completion_handler_type = typename async_result_type::completion_handler_type;
    completion_handler_type h(std::forward<CompletionToken>(token));
    async_result_type result(h);
    detail::poll_file_op op(std::move(i),
                            std::move(h));
    if constexpr (In) {
      async_poll_in(std::move(op));
    } else {
      async_poll_out(std::move(op));
    }
    return result.get();
  }
private:
  template<bool In,
           typename BufferSequence,
           typename Invoker,
           typename CompletionToken>
  auto async_impl(BufferSequence bs,
                  Invoker i,
                  CompletionToken&& token)
  {
    if (boost::asio::buffer_size(bs)) {
      return async_poll_then<In,
                             signature>(std::move(i),
                                        std::forward<CompletionToken>(token));
    }
    return post(std::forward<CompletionToken>(token),
                error_code(),
                std::size_t(0));
  }
public:
  /**
   *  Asynchronously reads from the file descriptor.
   *
   *  \tparam MutableBufferSequence
   *    A type which models `MutableBufferSequence`.
   *  \tparam CompletionToken
   *    A completion token whose associated completion handler
   *    is invocable with the following signature:
   *    \code
   *    void(std::error_code,
   *         std::size_t);
   *    \endcode
   *    Where the first argument is the result of the
   *    operation and the second argument is the number
   *    of bytes read. Note that in order to satisfy the
   *    `AsyncReadStream` concept as specified by Boost.Asio
   *    the final completion handler may alternately be
   *    invocable with the following signature:
   *    \code
   *    void(boost::system::error_code,
   *         std::size_t);
   *    \endcode
   *
   *  \param [in] mb
   *    The buffers into which to read bytes. Note that
   *    while this object itself will be copied as needed
   *    the underlying buffers must remain valid until
   *    the asynchronous operation completes or the behavior
   *    is undefined.
   *  \param [in] token
   *    A completion token to use to notify the caller of
   *    completion.
   *
   *  \return
   *    Whatever is appropriate given `CompletionToken` and
   *    `token`.
   */
  template<typename MutableBufferSequence,
           typename CompletionToken>
  auto async_read_some(MutableBufferSequence mb,
                       CompletionToken&& token)
  {
    return async_impl<true>(mb,
                            [fd = native_handle(),
                             mb](auto ec,
                                 auto h)
                            {
                              std::size_t bytes_transferred = 0;
                              if (!ec) {
                                bytes_transferred = asio::read(fd,
                                                               mb,
                                                               ec);
                              }
                              h(error_code(ec),
                                bytes_transferred);
                            },
                            std::forward<CompletionToken>(token));
  }
  /**
   *  Asynchronously writes to the file descriptor.
   *
   *  \tparam ConstBufferSequence
   *    A type which models `ConstBufferSequence`.
   *  \tparam CompletionToken
   *    A completion token whose associated completion handler
   *    is invocable with the following signature:
   *    \code
   *    void(std::error_code,
   *         std::size_t);
   *    \endcode
   *    Where the first argument is the result of the
   *    operation and the second argument is the number
   *    of bytes written. Note that in order to satisfy the
   *    `AsyncReadStream` concept as specified by Boost.Asio
   *    the final completion handler may alternately be
   *    invocable with the following signature:
   *    \code
   *    void(boost::system::error_code,
   *         std::size_t);
   *    \endcode
   *
   *  \param [in] cb
   *    The buffers from which to write bytes. Note that
   *    while this object itself will be copied as needed
   *    the underlying buffers must remain valid until
   *    the asynchronous operation completes or the behavior
   *    is undefined.
   *  \param [in] token
   *    A completion token to use to notify the caller of
   *    completion.
   *
   *  \return
   *    Whatever is appropriate given `CompletionToken` and
   *    `token`.
   */
  template<typename ConstBufferSequence,
           typename CompletionToken>
  auto async_write_some(ConstBufferSequence cb,
                        CompletionToken&& token)
  {
    return async_impl<false>(cb,
                            [fd = native_handle(),
                             cb](auto ec,
                                 auto h)
                            {
                              std::size_t bytes_transferred = 0;
                              if (!ec) {
                                bytes_transferred = asio::write(fd,
                                                                cb,
                                                                ec);
                              }
                              h(error_code(ec),
                                bytes_transferred);
                            },
                            std::forward<CompletionToken>(token));
  }
};

}

#ifndef ASIO_URING_DOXYGEN_RUNNING
namespace boost::asio {

template<typename CompletionHandler,
         typename Invoker,
         typename Allocator>
class associated_allocator<::asio_uring::asio::detail::poll_file_op<CompletionHandler,
                                                                    Invoker>,
                           Allocator>
{
public:
  using type = typename associated_allocator<CompletionHandler,
                                             Allocator>::type;

  static auto get(const ::asio_uring::asio::detail::poll_file_op<CompletionHandler,
                                                                 Invoker>& h,
                  const Allocator& alloc = Allocator())
  {
    return asio::get_associated_allocator(h.completion_handler(),
                                          alloc);
  }
};

template<typename CompletionHandler,
         typename Invoker,
         typename Executor>
class associated_executor<::asio_uring::asio::detail::poll_file_op<CompletionHandler,
                                                                   Invoker>,
                          Executor>
{
public:
  using type = typename associated_executor<CompletionHandler,
                                            Executor>::type;

  static auto get(const ::asio_uring::asio::detail::poll_file_op<CompletionHandler,
                                                                 Invoker>& h,
                  const Executor& ex = Executor())
  {
    return asio::get_associated_executor(h.completion_handler(),
                                         ex);
  }
};

}
#endif
