/**
 *  \file
 */

#pragma once

#include <cstddef>
#include <system_error>
#include <boost/asio/buffer.hpp>

namespace asio_uring::asio {

/**
 *  Writes as many bytes as possible to
 *  a file descriptor from a buffer without
 *  blocking.
 *
 *  This operation is not meaningful if
 *  the provided file descriptor is not
 *  non-blocking.
 *
 *  \param [in] fd
 *    The file descrptor.
 *  \param [in] buffer
 *    The buffer.
 *  \param [out] ec
 *    A `std::error_code` object which shall
 *    receive the result of the operation.
 *
 *  \return
 *    The number of bytes written.
 */
std::size_t write(int fd,
                  boost::asio::const_buffer buffer,
                  std::error_code& ec) noexcept;

template<typename ConstBufferSequence>
std::size_t write(int fd,
                  ConstBufferSequence cb,
                  std::error_code& ec)
{
  ec.clear();
  std::size_t retr = 0;
  for (auto begin = boost::asio::buffer_sequence_begin(cb), end = boost::asio::buffer_sequence_end(cb);
       begin != end;
       ++begin)
  {
    boost::asio::const_buffer buffer(*begin);
    auto written = asio::write(fd,
                               buffer,
                               ec);
    retr += written;
    if (ec) {
      return retr;
    }
    if (written != buffer.size()) {
      return retr;
    }
  }
  return retr;
}

}
