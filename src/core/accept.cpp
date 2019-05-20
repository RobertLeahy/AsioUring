#include <asio_uring/accept.hpp>

#include <cassert>
#include <optional>
#include <system_error>
#include <asio_uring/fd.hpp>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace asio_uring {

std::optional<fd> accept(int fd,
                         ::sockaddr* addr,
                         ::socklen_t addr_len,
                         std::error_code& ec) noexcept
{
#ifndef NDEBUG
  auto flags = ::fcntl(fd,
                       F_GETFL);
  assert((flags == -1) ||
         (flags & O_NONBLOCK));
#endif
  assert(bool(addr_len) == bool(addr));
  ec.clear();
  auto addr_len_ptr = addr ? &addr_len : nullptr;
  auto result = ::accept4(fd,
                          addr,
                          addr_len_ptr,
                          SOCK_NONBLOCK);
  if (result >= 0) {
    return asio_uring::fd(result);
  }
  if (!((errno == EAGAIN) ||
        (errno == EWOULDBLOCK)))
  {
    ec.assign(errno,
              std::generic_category());
  }
  return std::nullopt;
}

std::optional<fd> accept(int fd,
                         std::error_code& ec) noexcept
{
  return accept(fd,
                nullptr,
                0,
                ec);
}

}
