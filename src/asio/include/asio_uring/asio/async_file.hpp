/**
 *  \file
 */

#pragma once

#include <cstdint>
#include <utility>
#include "file_object.hpp"

namespace asio_uring::asio {

/**
 *  An I/O object which wraps a file descriptor for
 *  which the following `io_uring` operations are
 *  valid:
 *
 *  - `IORING_OP_READV`
 *  - `IORING_OP_WRITEV`
 *  - `IORING_OP_FSYNC`
 *
 *  Note that this describes file descriptors to
 *  actual file system files but notably does not
 *  describe:
 *
 *  - Sockets
 *  - Pipes
 *  - `eventfd` file descriptors
 *
 *  Among others.
 *
 *  This class models:
 *
 *  - `AsyncRandomAccessReadDevice`
 *  - `AsyncRandomAccessWriteDevice`
 *
 *  \sa
 *    poll_file
 */
class async_file : public file_object {
public:
  using file_object::file_object;
  /**
   *  Initiates an asynchronous read at a certain offset.
   *
   *  \tparam MutableBufferSequence
   *    A type which models `MutableBufferSequence` which
   *    is used to represent the area into which data
   *    which is read shall be written.
   *  \tparam CompletionToken
   *    A completion token whose associated completion handler
   *    is invocable with the following signature:
   *    \code
   *    void(std::error_code,
   *         std::size_t);
   *    \endcode
   *    Where the arguments are:
   *    1. The result of the operation
   *    2. The number of bytes read
   *
   *  \param [in] o
   *    The offset from the beginning of the file at which
   *    to perform the read.
   *  \param [in] mb
   *    The sequence of buffers into which to read. Note that
   *    this object will be copied as needed however the backing
   *    storage must remain valid for the lifetime of the asynchronous
   *    operation or the behavior is undefined.
   *  \param [in] token
   *    The completion token which shall be used to notify the
   *    caller of completion.
   *
   *  \return
   *    Whatever is appropriate given `CompletionToken` and
   *    `token`.
   */
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
  /**
   *  Initiates an asynchronous write at a certain offset.
   *
   *  \tparam ConstBufferSequence
   *    A type which models `ConstBufferSequence` which
   *    is used to represent the area from which data shall
   *    be written.
   *  \tparam CompletionToken
   *    A completion token whose associated completion handler
   *    is invocable with the following signature:
   *    \code
   *    void(std::error_code,
   *         std::size_t);
   *    \endcode
   *    Where the arguments are:
   *    1. The result of the operation
   *    2. The number of bytes written
   *
   *  \param [in] o
   *    The offset from the beginning of the file at which
   *    to perform the write.
   *  \param [in] cb
   *    The sequence of buffers from which to write. Note that
   *    this object will be copied as needed however the backing
   *    storage must remain valid for the lifetime of the asynchronous
   *    operation or the behavior is undefined.
   *  \param [in] token
   *    The completion token which shall be used to notify the
   *    caller of completion.
   *
   *  \return
   *    Whatever is appropriate given `CompletionToken` and
   *    `token`.
   */
  template<typename ConstBufferSequence,
           typename CompletionToken>
  auto async_write_some_at(std::uint64_t o,
                           ConstBufferSequence cb,
                           CompletionToken&& token)
  {
    return get_service().initiate_write_some_at(get_implementation(),
                                                native_handle(),
                                                o,
                                                cb,
                                                wrap_token(std::forward<CompletionToken>(token)));
  }
  /**
   *  Asynchronously performs `fsync` or `fdatasync` against
   *  the managed file descriptor.
   *
   *  \tparam CompletionToken
   *    A completion token whose associated completion handler
   *    is invocable with the following signature:
   *    \code
   *    void(std::error_code);
   *    \endcode
   *    Where the argument is the result of the operation.
   *
   *  \param [in] data_only
   *    `true` if the result of the operation shall be as if
   *    `fdatasync` were invoked, `false` for `fsync`.
   *  \param [in] token
   *    The completion token which shall be used to notify the
   *    caller of completion.
   *
   *  \return
   *    Whatever is appropriate given `CompletionToken` and
   *    `token`.
   */
  template<typename CompletionToken>
  auto async_flush(bool data_only,
                   CompletionToken&& token)
  {
    return get_service().initiate_fsync(get_implementation(),
                                        native_handle(),
                                        data_only,
                                        wrap_token(std::forward<CompletionToken>(token)));
  }
  /**
   *  Asynchronously performs `fsync` against the managed
   *  file descriptor.
   *
   *  \tparam CompletionToken
   *    A completion token whose associated completion handler
   *    is invocable with the following signature:
   *    \code
   *    void(std::error_code);
   *    \endcode
   *    Where the argument is the result of the operation.
   *
   *  \param [in] token
   *    The completion token which shall be used to notify the
   *    caller of completion.
   *
   *  \return
   *    Whatever is appropriate given `CompletionToken` and
   *    `token`.
   */
  template<typename CompletionToken>
  auto async_flush(CompletionToken&& token) {
    return async_flush(false,
                       std::forward<CompletionToken>(token));
  }
};

}
