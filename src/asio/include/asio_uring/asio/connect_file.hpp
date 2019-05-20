/**
 *  \file
 */

#pragma once

#include <system_error>
#include <utility>
#include <asio_uring/connect.hpp>
#include "poll_file.hpp"

namespace asio_uring::asio {

class connect_file : public poll_file {
private:
  using signature = void(std::error_code);
public:
  using poll_file::poll_file;
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
