/**
 *  \file
 */

#pragma once

#include <type_traits>
#include <boost/asio/buffer.hpp>
#include <sys/uio.h>

namespace asio_uring::asio {

::iovec to_iovec(boost::asio::const_buffer cb) noexcept;
::iovec to_iovec(boost::asio::mutable_buffer mb) noexcept;

template<typename BufferSequence>
void to_iovecs(BufferSequence b,
               ::iovec* iovs) noexcept
{
  static_assert(boost::asio::is_const_buffer_sequence<BufferSequence>::value ||
                boost::asio::is_mutable_buffer_sequence<BufferSequence>::value);
  using buffer_type = std::conditional_t<boost::asio::is_const_buffer_sequence<BufferSequence>::value,
                                         boost::asio::const_buffer,
                                         boost::asio::mutable_buffer>;
  for (auto begin = boost::asio::buffer_sequence_begin(b), end = boost::asio::buffer_sequence_end(b);
       begin != end;
       ++begin, ++iovs)
  {
    buffer_type buffer(*begin);
    *iovs = to_iovec(buffer);
  }
}

}
