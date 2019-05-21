/**
 *  \file
 */

#pragma once

#include <system_error>
#include <utility>
#include <asio_uring/connect.hpp>
#include "poll_file.hpp"

namespace asio_uring::asio {

/**
 *  An I/O object for file descriptors for which
 *  the `connect` POSIX function is valid and/or
 *  meaningful.
 */
class connect_file : public poll_file {
private:
  using signature = void(std::error_code);
public:
  using poll_file::poll_file;
  /**
   *  Initiates an asynchronous connection attempt.
   *
   *  \tparam Address
   *    The type of object used to represent the
   *    destination address.
   *  \tparam CompletionToken
   *    A completion token whose associated completion
   *    handler is invocable with the following signature:
   *    \code
   *    void(std::error_code);
   *    \endcode
   *    Where the sole argument gives the result of the
   *    operation.
   *
   *  \param [in] addr
   *    The object which represents the destination address.
   *  \param [in] token
   *    A completion token which shall be used to notify the
   *    caller of completion.
   *
   *  \return
   *    Whatever is appropriate given `CompletionToken` and
   *    `token`.
   */
  template<typename Address,
           typename CompletionToken>
  auto async_connect(const Address& addr,
                     CompletionToken&& token)
  {
    std::error_code ec;
    bool connected = asio_uring::connect(native_handle(),
                                         addr,
                                         ec);
    if (ec || connected) {
      return post(std::forward<CompletionToken>(token),
                  ec);
    }
    auto i = [fd = native_handle()](auto ec,
                                    auto h)
    {
      if (!ec) {
        ec = connect_error(fd);
      }
      h(ec);
    };
    return async_poll_then<false,
                           signature>(i,
                                      std::forward<CompletionToken>(token));
  }
};

}
