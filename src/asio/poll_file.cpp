#include <asio_uring/asio/poll_file.hpp>

#include <system_error>
#include <utility>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

namespace asio_uring::asio {

poll_file::poll_file(execution_context& ctx,
                     fd file)
  : file_object(ctx,
                std::move(file))
{
  auto flags = ::fcntl(native_handle(),
                       F_GETFL);
  if (flags == -1) {
    std::error_code ec(errno,
                       std::generic_category());
    throw std::system_error(ec);
  }
  flags |= O_NONBLOCK;
  if (::fcntl(native_handle(),
              F_SETFL,
              flags) == -1)
  {
    std::error_code ec(errno,
                       std::generic_category());
    throw std::system_error(ec);
  }
}

}
