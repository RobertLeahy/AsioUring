#include <asio_uring/read.hpp>

#include <cassert>
#include <cstddef>
#include <system_error>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

namespace asio_uring {

std::size_t read(int fd,
                 void* ptr,
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
    auto result = ::read(fd,
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
    std::size_t read(result);
    retr += read;
    ptr = static_cast<char*>(ptr) + read;
    size -= read;
  }
  return retr;
}

}
