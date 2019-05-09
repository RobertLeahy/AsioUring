#include <asio_uring/fd.hpp>

#include <cassert>
#include <system_error>
#include <errno.h>
#include <unistd.h>

namespace asio_uring {

namespace {

void maybe_close(fd::const_native_handle_type fd) noexcept {
  if (fd != -1) {
    int result = ::close(fd);
    assert(!result);
    (void)result;
  }
}

}

fd::fd() noexcept
  : handle_(-1)
{}

fd::fd(const_native_handle_type handle)
  : handle_(handle)
{
  if (handle_ == -1) {
    std::error_code ec(errno,
                       std::generic_category());
    throw std::system_error(ec);
  }
}

fd::fd(fd&& other) noexcept
  : handle_(other.handle_)
{
  other.handle_ = -1;
}

fd& fd::operator=(fd&& other) noexcept {
  assert(&other != this);
  maybe_close(handle_);
  handle_ = other.handle_;
  other.handle_ = -1;
  return *this;
}

fd::~fd() noexcept {
  maybe_close(handle_);
}

fd::const_native_handle_type fd::native_handle() const noexcept {
  return handle_;
}

}
