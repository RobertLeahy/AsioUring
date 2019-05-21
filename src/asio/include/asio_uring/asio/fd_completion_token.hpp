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
#include <boost/asio/async_result.hpp>
#include "fd_completion_handler.hpp"

namespace asio_uring::asio {

/**
 *  A completion token wrapper which:
 *
 *  - Maintains the value returned from the
 *    initiating function
 *  - Applies the same transformation to the
 *    final completion handler as is performed
 *    by \ref fd_completion_handler
 *
 *  \tparam CompletionToken
 *    The completion token type to wrap.
 *
 *  \sa
 *    fd_completion_handler
 */
template<typename CompletionToken>
class fd_completion_token {
public:
#ifndef ASIO_URING_DOXYGEN_RUNNING
  using completion_token_type = CompletionToken;
#endif
  /**
   *  A type alias for `std::shared_ptr<fd>`.
   */
  using fd_type = std::shared_ptr<asio_uring::fd>;
  /**
   *  Wraps a completion token and \ref fd "file descriptor".
   *
   *  \param [in] token
   *    The completion token.
   *  \param [in] fd
   *    The `std::shared_ptr` to the \ref fd "file descripctor".
   *    If `!fd` is `true` the behavior is undefined.
   */
  explicit fd_completion_token(CompletionToken token,
                               fd_type fd) noexcept(std::is_nothrow_move_constructible_v<CompletionToken>)
    : token_(std::move(token)),
      fd_   (std::move(fd))
  {}
#ifndef ASIO_URING_DOXYGEN_RUNNING
  CompletionToken&& completion_token() && noexcept {
    return std::move(token_);
  }
  fd_type&& fd() && noexcept {
    return std::move(fd_);
  }
#endif
private:
  CompletionToken token_;
  fd_type         fd_;
};

namespace detail {

template<typename CompletionHandler>
class fd_completion_token_handler : public fd_completion_handler<CompletionHandler> {
public:
  template<typename CompletionToken>
  fd_completion_token_handler(fd_completion_token<CompletionToken> token) noexcept(std::is_nothrow_constructible_v<CompletionHandler,
                                                                                                                   CompletionToken&&>)
    : fd_completion_handler<CompletionHandler>(CompletionHandler(std::move(token).completion_token()),
                                               std::move(token).fd())
  {}
};

}

}

#ifndef ASIO_URING_DOXYGEN_RUNNING
namespace boost::asio {

template<typename CompletionToken,
         typename Signature>
class async_result<::asio_uring::asio::fd_completion_token<CompletionToken>,
                   Signature>
{
private:
  using async_result_type = async_result<CompletionToken,
                                         Signature>;
  using inner_completion_handler_type = typename async_result_type::completion_handler_type;
  using completion_token_type = ::asio_uring::asio::fd_completion_token<CompletionToken>;
  using fd_type = typename completion_token_type::fd_type;
public:
  using return_type = typename async_result_type::return_type;
  using completion_handler_type = ::asio_uring::asio::detail::fd_completion_token_handler<inner_completion_handler_type>;
  async_result(completion_handler_type& h) noexcept(std::is_nothrow_constructible_v<async_result_type,
                                                                                    inner_completion_handler_type&>)
    : inner_(h.completion_handler())
  {}
  return_type get() noexcept(noexcept(std::declval<async_result_type&>().get())) {
    return inner_.get();
  }
private:
  async_result_type inner_;
};

template<typename CompletionHandler,
         typename Allocator>
class associated_allocator<::asio_uring::asio::detail::fd_completion_token_handler<CompletionHandler>,
                           Allocator> : public associated_allocator<::asio_uring::asio::fd_completion_handler<CompletionHandler>,
                                                                    Allocator>
{};

template<typename CompletionHandler,
         typename Executor>
class associated_executor<::asio_uring::asio::detail::fd_completion_token_handler<CompletionHandler>,
                          Executor> : public associated_executor<::asio_uring::asio::fd_completion_handler<CompletionHandler>,
                                                                 Executor>
{};

}
#endif
