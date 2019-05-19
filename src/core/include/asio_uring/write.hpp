/**
 *  \file
 */

#pragma once

#include <cstddef>
#include <system_error>

namespace asio_uring {

/**
 *  Writes from a buffer until either:
 *
 *  - The write operation indicates that
 *    data could not be written without
 *  - blocking
 *  - The buffer is empty
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
 *    A pointer to the buffer from which to
 *    write.
 *  \param [in] size
 *    The number of bytes which may be written
 *    through `ptr` at maximum.
 *  \param [out] ec
 *    A `std::error_code` which shall receive the
 *    result of the operation.
 *
 *  \return
 *    The number of bytes written.
 */
std::size_t write(int fd,
                  const void* ptr,
                  std::size_t size,
                  std::error_code& ec) noexcept;

}
