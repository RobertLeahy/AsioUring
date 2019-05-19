/**
 *  \file
 */

#pragma once

#include <cstddef>
#include <system_error>
#include <boost/asio/buffer.hpp>

namespace asio_uring::asio {

/**
 *  Attempts to read as many bytes
 *  as possible into a buffer without
 *  blocking.
 *
 *  Note that in order for this operation
 *  to be meaningful the provided file
 *  descriptor must bo non-blocking.
 *
 *  \param [in] fd
 *    The file descriptor.
 *  \param [in] buffer
 *    The buffer into which to read.
 *  \param [out] ec
 *    A `std::error_code` which shall receive
 *    the result of the operation.
 *
 *  \return
 *    The number of bytes read.
 */
std::size_t read(int fd,
                 boost::asio::mutable_buffer buffer,
                 std::error_code& ec) noexcept;

/**
 *  Reads as many bytes as possible into
 *  a sequence of buffers without blocking.
 *
 *  Note that in order for this operation to
 *  be meaningful the provided file descriptor
 *  must be non-blocking.
 *
 *  \tparam MutableBufferSequence
 *    A type which models `MutableBufferSequence`.
 *
 *  \param [in] fd
 *    The file descriptor.
 *  \param [in] mb
 *    The sequence of buffers into which to read.
 *  \param [out] ec
 *    A `std::error_code` which shall receive the
 *    result of the operation.
 *
 *  \return
 *    The number of bytes read.
 */
template<typename MutableBufferSequence>
std::size_t read(int fd,
                 MutableBufferSequence mb,
                 std::error_code& ec)
{
  ec.clear();
  std::size_t retr = 0;
  for (auto begin = boost::asio::buffer_sequence_begin(mb), end = boost::asio::buffer_sequence_end(mb);
       begin != end;
       ++begin)
  {
    boost::asio::mutable_buffer buffer(*begin);
    auto read = asio::read(fd,
                           buffer,
                           ec);
    retr += read;
    if (ec) {
      return retr;
    }
    if (read != buffer.size()) {
      return retr;
    }
  }
  return retr;
}

}
