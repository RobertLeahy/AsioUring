/**
 *  \file
 */

#pragma once

#include <type_traits>
#include <boost/asio/buffer.hpp>
#include <sys/uio.h>

namespace asio_uring::asio {

/**
 *  Obtains an `::iovec` which indicates the
 *  same buffer as a `boost::asio::const_buffer`.
 *
 *  \warning
 *    This function violates const correctness since
 *    `::iovec::iov_base` is cv-unqualified. Care must
 *    be taken to not write through the pointer stored
 *    in `::iovec::iov_base` in the returned value (if
 *    this happens undefined behavior may result).
 *
 *  \param [in] cb
 *    The buffer.
 *
 *  \return
 *    A `::iovec` which indicates the same buffer as
 *    `cb`.
 */
::iovec to_iovec(boost::asio::const_buffer cb) noexcept;

/**
 *  Obtains an `::iovec` which indicates the
 *  same buffer as a `boost::asio::mutable_buffer`.
 *
 *  \param [in] mb
 *    The buffer.
 *
 *  \return
 *    A `::iovec` which indicates the same buffer as
 *    `mb`.
 */
::iovec to_iovec(boost::asio::mutable_buffer mb) noexcept;

/**
 *  Transforms a sequence of `boost::asio::const_buffer` or
 *  `boost::asio::mutable_buffer` objects into a sequence
 *  of `::iovec` objects.
 *
 *  \tparam BufferSequence
 *    A model of `MutableBufferSequence` or
 *    `ConstBufferSequence`.
 *
 *  \param [in] b
 *    A buffer sequence.
 *  \param [out] iovs
 *    The sequence of `::iovec` objects to write. It must
 *    be possible to write a number of `::iovec` objects
 *    through this pointer which is equal to:
 *    \code
 *    std::distance(boost::asio::buffer_sequence_begin(b),
 *                  boost::asio::buffer_sequence_end(b));
 *    \endcode
 *    or the behavior is undefined.
 */
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
