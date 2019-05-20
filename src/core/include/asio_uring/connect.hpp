/**
 *  \file
 */

#pragma once

#include <memory>
#include <system_error>
#include <sys/socket.h>
#include <sys/types.h>

namespace asio_uring {

/**
 *  Initiates a non-blocking connection
 *  attempt.
 *
 *  Only meaningful for file descriptors
 *  in non-blocking mode.
 *
 *  \param [in] fd
 *    The file descriptor.
 *  \param [in] addr
 *    A pointer to the address structure
 *    type punned as a `::sockaddr`. Must
 *    not be `nullptr`.
 *  \param [in] addr_len
 *    The number of bytes which may be read
 *    through `addr`. Must not be `0`.
 *  \param [out] ec
 *    A `std::error_code` which shall receive
 *    the result of the operation.
 *
 *  \return
 *    `true` if the connection was made
 *    immediately. `false` if the caller must
 *    fall back to waiting for the file
 *    descriptor to be waitable to determine
 *    whether or not the connection attempt
 *    was successful. Note that if `ec` is
 *    truthy after the call returns this
 *    value is meaningless.
 */
bool connect(int fd,
             const ::sockaddr* addr,
             ::socklen_t addr_len,
             std::error_code& ec) noexcept;

/**
 *  Initiates a non-blocking connection
 *  attempt.
 *
 *  Only meaningful for file descriptors
 *  in non-blocking mode.
 *
 *  \tparam Address
 *    The type of structure which shall be
 *    used to indicate the address to
 *    connect to.
 *
 *  \param [in] fd
 *    The file descriptor.
 *  \param [in] addr
 *    The structure which indicates the address
 *    to which to connect.
 *  \param [out] ec
 *    A `std::error_code` which shall receive
 *    the result of the operation.
 *
 *  \return
 *    `true` if the connection was made
 *    immediately. `false` if the caller must
 *    fall back to waiting for the file
 *    descriptor to be waitable to determine
 *    whether or not the connection attempt
 *    was successful. Note that if `ec` is
 *    truthy after the call returns this
 *    value is meaningless.
 */
template<typename Address>
bool connect(int fd,
             const Address& addr,
             std::error_code& ec) noexcept
{
  return connect(fd,
                 reinterpret_cast<const ::sockaddr*>(std::addressof(addr)),
                 sizeof(addr),
                 ec);
}

/**
 *  Retrieves the result of a connect
 *  operation which did not complete
 *  immediately.
 *
 *  May be called only once the socket
 *  becomes writable.
 *
 *  \param [in] fd
 *    The file descriptor to query.
 *
 *  \return
 *    A `std::error_code` encapsulating
 *    the result both of this operation and
 *    of the asynchronous connect.
 */
std::error_code connect_error(int fd) noexcept;

}
