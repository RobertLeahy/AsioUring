/**
 *  \file
 */

#pragma once

#include <cassert>
#include <memory>
#include <optional>
#include <system_error>
#include <utility>
#include <asio_uring/accept.hpp>
#include <asio_uring/fd.hpp>
#include "poll_file.hpp"

namespace asio_uring::asio {

/**
 *  An I/O object which wraps a file descriptor for
 *  which the POSIX `accept` (and Linux `accept4`) calls
 *  are valid and meaningful.
 */
class accept_file {
public:
  accept_file() = delete;
  accept_file(const accept_file&) = delete;
  accept_file(accept_file&&) = default;
  accept_file& operator=(const accept_file&) = delete;
  accept_file& operator=(accept_file&&) = default;
  /**
   *  Creates an accept_file by assuming ownership of
   *  a certain file descriptor.
   *
   *  \param [in] ctx
   *    The \ref execution_context to use for asynchronous
   *    I/O. This reference must remain valid for the
   *    lifetime of the object or the behavior is undefined.
   *  \param [in] file
   *    An owning wrapper to the file descriptor to assume
   *    ownership of. It is assumed that `O_NONBLOCK` is set
   *    on this file descriptor. If this is not the case the
   *    behavior is undefined.
   */
  accept_file(execution_context& ctx,
              fd file);
private:
  using signature = void(std::error_code,
                         fd);
  class impl : public poll_file,
               public std::enable_shared_from_this<impl>
  {
  public:
    using poll_file::poll_file;
    template<typename Address,
             typename CompletionToken>
    auto async_accept(Address* addr,
                      CompletionToken&& token)
    {
      auto i = [addr,
                self = shared_from_this()](auto ec,
                                           auto h) mutable
      {
        if (ec) {
          h(ec,
            fd());
          return;
        }
        std::optional<fd> accepted;
        if (addr) {
          accepted = accept(self->native_handle(),
                            *addr,
                            ec);
        } else {
          accepted = accept(self->native_handle(),
                            ec);
        }
        if (ec) {
          h(ec,
            fd());
          return;
        }
        if (accepted) {
          h(ec,
            std::move(*accepted));
          return;
        }
        self->async_accept(addr,
                           std::move(h));
      };
      return async_poll_then<true,
                             accept_file::signature>(std::move(i),
                                                     std::forward<CompletionToken>(token));
    }
  };
public:
  /**
   *  Type alias for \ref asio_uring::execution_context::executor_type "execution_context::executor_type".
   */
  using executor_type = impl::executor_type;
  /**
   *  Type alias for \ref asio_uring::service::implementation_type "service::implementation_type".
   */
  using implementation_type = impl::implementation_type;
  /**
   *  Type alias for \ref service.
   */
  using service_type = impl::service_type;
  /**
   *  Retrieves the I/O executor for this object.
   *
   *  \return
   *    An \ref executor_type "executor".
   */
  executor_type get_executor() const noexcept;
  /**
   *  @{
   *  Retrieves the wrapped \ref implementation_type "implementation handle".
   *
   *  \return
   *    A reference to the wrapped \ref implementation_type "implementation handle".
   */
  implementation_type& get_implementation() noexcept;
  const implementation_type& get_implementation() const noexcept;
  /**
   *  @}
   */
  service_type& get_service() noexcept;
  /**
   *  Initiates an asynchronous operation which has the
   *  same effect as the following synchronous call:
   *  \code
   *  accept4(native_handle(),
   *          nullptr,
   *          nullptr,
   *          O_NONBLOCK);
   *  \endcode
   *
   *  \tparam CompletionToken
   *    A completion token whose associated completion
   *    handler is invocable with the following signature:
   *    \code
   *    void(std::error_code,
   *         fd);
   *    \endcode
   *    Where the arguments are:
   *    1. The result of the operation
   *    2. An owning wrapper for the newly-accepted file
   *       descriptor (note that this argument is not
   *       meaningful if the first argument is truthy)
   *
   *  \param [in] token
   *    The completion token to use to notify the caller
   *    of asynchronous completion.
   *
   *  \return
   *    Whatever is appropriate given `CompletionToken` and
   *    `token`.
   */
  template<typename CompletionToken>
  auto async_accept(CompletionToken&& token) {
    assert(impl_);
    char* ptr = nullptr;
    return impl_->async_accept(ptr,
                               std::forward<CompletionToken>(token));
  }
  /**
   *  Initiates an asynchronous operation which has the
   *  same effect as the following synchronous call:
   *  \code
   *  ::socklen_t addrlen = sizeof(addr);
   *  accept4(native_handle(),
   *          reinterpret_cast<::sockaddr*>(std::addressof(addr)),
   *          &addrlen,
   *          O_NONBLOCK);
   *  \endcode
   *
   *  \tparam Address
   *    The type of object used to represent the address
   *    from which a connection was accepted.
   *  \tparam CompletionToken
   *    A completion token whose associated completion
   *    handler is invocable with the following signature:
   *    \code
   *    void(std::error_code,
   *         fd);
   *    \endcode
   *    Where the arguments are:
   *    1. The result of the operation
   *    2. An owning wrapper for the newly-accepted file
   *       descriptor (note that this argument is not
   *       meaningful if the first argument is truthy)
   *
   *  \param [out] addr
   *    An object used to represent the address from which
   *    the connection is accepted. This reference must remain
   *    valid for the lifetime of the asynchronous operation or
   *    the behavior is undefined.
   *  \param [in] token
   *    The completion token to use to notify the caller
   *    of asynchronous completion.
   *
   *  \return
   *    Whatever is appropriate given `CompletionToken` and
   *    `token`.
   */
  template<typename Address,
           typename CompletionToken>
  auto async_accept(Address& addr,
                    CompletionToken&& token)
  {
    assert(impl_);
    return impl_->async_accept(std::addressof(addr),
                               std::forward<CompletionToken>(token));
  }
private:
  using impl_type = std::shared_ptr<impl>;
  impl_type impl_;
};

}
