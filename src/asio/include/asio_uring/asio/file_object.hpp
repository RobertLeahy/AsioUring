/**
 *  \file
 */

#pragma once

#include <memory>
#include <type_traits>
#include <asio_uring/fd.hpp>
#include "basic_io_object.hpp"
#include "execution_context.hpp"
#include "fd_completion_handler.hpp"
#include "fd_completion_token.hpp"
#include "service.hpp"

namespace asio_uring::asio {

/**
 *  Derives from \ref basic_io_object and provides
 *  convenience functionality for implementing an
 *  I/O object which initiates asynchronous operations
 *  on a \ref fd "file descriptor" against a
 *  \ref uring "io_ring".
 */
class file_object : public basic_io_object<execution_context,
                                           service>
{
private:
  using base = basic_io_object<execution_context,
                               service>;
public:
  /**
   *  A type alias for \ref fd::native_handle_type.
   */
  using native_handle_type = fd::native_handle_type;
  /**
   *  A type alias for \ref fd::const_native_handle_type.
   */
  using const_native_handle_type = fd::const_native_handle_type;
  /**
   *  Creates a file_object which is associated with
   *  a certain \ref execution_context "execution context"
   *  and which wraps a certain \ref fd "file descriptor".
   *
   *  \param [in] ctx
   *    The \ref execution_context "execution context". This
   *    reference must remain valid until the end of this
   *    object's lifetime or the behavior is undefined.
   *  \param [in] file
   *    The \ref fd "file descriptor" to wrap. Must not be an
   *    invalid file descriptor or the behavior is undefined.
   */
  file_object(execution_context& ctx,
              fd file);
  /** 
   *  @{
   *  Obtains the raw file descripctor.
   *
   *  \return
   *    The raw file descripctor.
   */
  native_handle_type native_handle() noexcept;
  const_native_handle_type native_handle() const noexcept;
  /**
   *  @}
   */
protected:
  /**
   *  Wraps a completion handler such that all properties
   *  of the completion handler are preserved and such
   *  that the \ref fd "file descriptor" associated with
   *  this object has its lifetime extended until the
   *  invocation or destruction of the wrapped completion
   *  handler (whichever comes first).
   *
   *  \tparam CompletionHandler
   *    The completion handler type to wrap.
   *
   *  \param [in] h
   *    The completion handler to wrap.
   *
   *  \return
   *    A \ref fd_completion_handler which wraps `h` and
   *    the \ref fd "file descripctor" owned by this
   *    object.
   */
  template<typename CompletionHandler>
  auto wrap_handler(CompletionHandler&& h) noexcept(std::is_nothrow_move_constructible_v<std::decay_t<CompletionHandler>>) {
    return fd_completion_handler(std::forward<CompletionHandler>(h),
                                 fd_);
  }
  /**
   *  Wraps a completion token such that all properties
   *  of the completion token are preserved and such
   *  that the \ref fd "file descriptor" associated with
   *  this object has its lifetime extended until the
   *  invocation or destruction of the final completion
   *  handler (or the destruction thereof, whichever
   *  comes first).
   *
   *  \tparam CompletionToken
   *    The completion token type to wrap.
   *
   *  \param [in] token
   *    The completion token to wrap.
   *
   *  \return
   *    A \ref fd_completion_token which wraps `token`
   *    and the \ref fd "file descripctor" owned by this
   *    object.
   */
  template<typename CompletionToken>
  auto wrap_token(CompletionToken&& token) noexcept(std::is_nothrow_move_constructible_v<std::decay_t<CompletionToken>>) {
    return fd_completion_token(std::forward<CompletionToken>(token),
                               fd_);
  }
  /**
   *  Releases ownership of the owned
   *  \ref fd "file descripctor".
   *
   *  Note that completion handlers obtained by
   *  \ref wrap_handler and final completion
   *  handlers obtained from completion tokens
   *  obtained from \ref wrap_token may cause
   *  the lifetime of the \ref fd "file descriptor"
   *  to persist past the return from this call.
   *
   *  Once this call returns it is no longer
   *  valid to call \ref outstanding.
   */
  void reset() noexcept;
  /**
   *  Obtains the number of pending completion
   *  handlers (including final completion handlers
   *  obtained from completion tokens generated
   *  by \ref wrap_token).
   *
   *  If \ref reset has been called on this object
   *  the behavior is undefined.
   *
   *  \return
   *    The number of pending completions.
   */
  long outstanding() const noexcept;
private:
  std::shared_ptr<fd> fd_;
};

}
