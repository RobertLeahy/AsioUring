/**
 *  \file
 */

#pragma once

#include <cstdint>
#include <utility>
#include "file_object.hpp"

namespace asio_uring::asio {

class async_file : public file_object {
public:
  using file_object::file_object;
  template<typename MutableBufferSequence,
           typename CompletionToken>
  auto async_read_some_at(std::uint64_t o,
                          MutableBufferSequence mb,
                          CompletionToken&& token)
  {
    return get_service().initiate_read_some_at(get_implementation(),
                                               native_handle(),
                                               o,
                                               mb,
                                               wrap_token(std::forward<CompletionToken>(token)));
  }
  template<typename ConstBufferSequence,
           typename CompletionToken>
  auto async_write_some_at(std::uint64_t o,
                           ConstBufferSequence mb,
                           CompletionToken&& token)
  {
    return get_service().initiate_write_some_at(get_implementation(),
                                                native_handle(),
                                                o,
                                                mb,
                                                wrap_token(std::forward<CompletionToken>(token)));
  }
  template<typename CompletionToken>
  auto async_flush(bool data_only,
                   CompletionToken&& token)
  {
    return get_service().initiate_fsync(get_implementation(),
                                        native_handle(),
                                        data_only,
                                        wrap_token(std::forward<CompletionToken>(token)));
  }
  template<typename CompletionToken>
  auto async_flush(CompletionToken&& token) {
    return async_flush(false,
                       std::forward<CompletionToken>(token));
  }
};

}
