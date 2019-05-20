/**
 *  \file
 */

#pragma once

#include <memory>
#include <optional>
#include <system_error>
#include "fd.hpp"
#include <sys/socket.h>
#include <sys/types.h>

namespace asio_uring {

/**
 *  Performs a non-blocking accept on a socket.
 *
 *  Note that in order for this operation to be
 *  meaningful the provided file descriptor must
 *  be non-blocking.
 *
 *  \param [in] fd
 *    The file descriptor.
 *  \param [out] addr
 *    A pointer to the address structure (type punned as
 *    a `::sockaddr`) which shall receive the address
 *    from which a connection was accepted. May be `nullptr`
 *    in which case this information is not provided.
 *  \param [in] addr_len
 *    The number of bytes which may be written through
 *    `addr`.
 *  \param [out] ec
 *    A `std::error_code` which shall receive the result
 *    of the operation.
 *
 *  \return
 *    A `std::optional` containing a \ref fd which
 *    encapsulates a file descriptor to the accepted
 *    connection if a connection was accepted, an empty
 *    `std::optional` otherwise. Note that if `ec` is
 *    truthy after the call returns the returned value
 *    is not meaningful.
 */
std::optional<fd> accept(int fd,
                         ::sockaddr* addr,
                         ::socklen_t addr_len,
                         std::error_code& ec) noexcept;

/**
 *  Performs a non-blocking accept on a socket.
 *
 *  Note that in order for this operation to be
 *  meaningful the provided file descriptor must
 *  be non-blocking.
 *
 *  \tparam Address
 *    The type which shall be used to indicate
 *    the remote address.
 *
 *  \param [in] fd
 *    The file descriptor.
 *  \param [out] addr
 *    The address structure which shall receive the address
 *    from which a connection was accepted.
 *  \param [out] ec
 *    A `std::error_code` which shall receive the result
 *    of the operation.
 *
 *  \return
 *    A `std::optional` containing a \ref fd which
 *    encapsulates a file descriptor to the accepted
 *    connection if a connection was accepted, an empty
 *    `std::optional` otherwise. Note that if `ec` is
 *    truthy after the call returns the returned value
 *    is not meaningful.
 */
template<typename Address>
std::optional<fd> accept(int fd,
                         Address& addr,
                         std::error_code& ec) noexcept
{
  return accept(fd,
                reinterpret_cast<::sockaddr*>(std::addressof(addr)),
                sizeof(addr),
                ec);
}

/**
 *  Performs a non-blocking accept on a socket.
 *
 *  Note that in order for this operation to be
 *  meaningful the provided file descriptor must
 *  be non-blocking.
 *
 *  \param [in] fd
 *    The file descriptor.
 *  \param [out] ec
 *    A `std::error_code` which shall receive the result
 *    of the operation.
 *
 *  \return
 *    A `std::optional` containing a \ref fd which
 *    encapsulates a file descriptor to the accepted
 *    connection if a connection was accepted, an empty
 *    `std::optional` otherwise. Note that if `ec` is
 *    truthy after the call returns the returned value
 *    is not meaningful.
 */
std::optional<fd> accept(int fd,
                         std::error_code& ec) noexcept;

}
