#include <asio_uring/connect.hpp>

#include <cassert>
#include <system_error>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace asio_uring {

bool connect(int fd,
             const ::sockaddr* addr,
             ::socklen_t addr_len,
             std::error_code& ec) noexcept
{
  assert(addr);
  assert(addr_len);
#ifndef NDEBUG
  auto flags = ::fcntl(fd,
                       F_GETFL);
  assert((flags == -1) ||
         (flags & O_NONBLOCK));
#endif
  ec.clear();
  auto result = ::connect(fd,
                          addr,
                          addr_len);
  if (result == 0) {
    return true;
  }
  if ((errno == EAGAIN) ||
      (errno == EINPROGRESS))
  {
    return false;
  }
  ec.assign(errno,
            std::generic_category());
  return false;
}

std::error_code connect_error(int fd) noexcept {
  int e;
  ::socklen_t len = sizeof(e);
  auto result = ::getsockopt(fd,
                             SOL_SOCKET,
                             SO_ERROR,
                             &e,
                             &len);
  if (result == 0) {
    assert(len == sizeof(e));
    return std::error_code(e,
                           std::generic_category());
  }
  return std::error_code(errno,
                         std::generic_category());
}

}
