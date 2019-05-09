#include <asio_uring/uring.hpp>

#include <system_error>
#include <asio_uring/liburing.hpp>
#include <errno.h>

namespace asio_uring {

uring::uring(unsigned entries,
             unsigned flags)
{
  if (::io_uring_queue_init(entries,
                            &ring_,
                            flags))
  {
    std::error_code ec(errno,
                       std::generic_category());
    throw std::system_error(ec);
  }
}

uring::~uring() noexcept {
  ::io_uring_queue_exit(&ring_);
}

uring::native_handle_type uring::native_handle() noexcept {
  return &ring_;
}

uring::const_native_handle_type uring::native_handle() const noexcept {
  return &ring_;
}

}
