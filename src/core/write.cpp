#include <asio_uring/write.hpp>

#include <cassert>
#include <cstddef>
#include <system_error>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

namespace asio_uring {

std::size_t write(int fd,
                  const void* ptr,
                  std::size_t size,
                  std::error_code& ec) noexcept
{
#ifndef NDEBUG
  auto flags = ::fcntl(fd,
                       F_GETFL);
  assert((flags == -1) ||
         (flags & O_NONBLOCK));
#endif
  ec.clear();
  std::size_t retr = 0;
  while (size) {
    auto result = ::write(fd,
                          ptr,
                          size);
    if (result == -1) {
      if (!((errno == EAGAIN) ||
            (errno == EWOULDBLOCK)))
      {
        ec.assign(errno,
                  std::generic_category());
      }
      return retr;
    }
    std::size_t written(result);
    retr += written;
    ptr = static_cast<const char*>(ptr) + written;
    size -= written;
  }
  return retr;
}

}
