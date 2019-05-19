/**
 *  \file
 */

#pragma once

#include <cstddef>
#include <system_error>

namespace asio_uring {

/**
 *  Reads into a buffer until either:
 *
 *  - The read operation indicates that
 *    data could not be read without blocking
 *  - The buffer is full
 *
 *  Note that the former condition is not
 *  meaningful unless the file descriptor is
 *  non-blocking and that therefore this
 *  operation assumes this about the provided
 *  file descriptor.
 *
 *  \param [in] fd
 *    The file descriptor.
 *  \param [in] ptr
 *    A pointer to the buffer into which to
 *    read.
 *  \param [in] size
 *    The number of bytes which may be read into
 *    `ptr` at maximum.
 *  \param [out] ec
 *    A `std::error_code` which shall receive the
 *    result of the operation.
 *
 *  \return
 *    The number of bytes read.
 */
std::size_t read(int fd,
                 void* ptr,
                 std::size_t size,
                 std::error_code& ec) noexcept;

}
