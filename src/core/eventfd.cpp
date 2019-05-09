#include <asio_uring/eventfd.hpp>

#include <cassert>
#include <system_error>
#include <errno.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace asio_uring {

eventfd::eventfd(unsigned int initval,
                 int flags)
  : fd(::eventfd(initval,
                 flags))
{}

eventfd::integer_type eventfd::read() {
  std::error_code ec;
  integer_type result = read(ec);
  if (ec) {
    throw std::system_error(ec);
  }
  return result;
}

eventfd::integer_type eventfd::read(std::error_code& ec) noexcept {
  ec.clear();
  integer_type retr;
  ::ssize_t bytes = ::read(native_handle(),
                           &retr,
                           sizeof(retr));
  if (bytes < 0) {
    ec.assign(errno,
              std::generic_category());
    return 0;
  }
  assert(bytes == sizeof(retr));
  return retr;
}

void eventfd::write(integer_type add) {
  std::error_code ec;
  write(add,
        ec);
  if (ec) {
    throw std::system_error(ec);
  }
}

void eventfd::write(integer_type add,
                    std::error_code& ec) noexcept
{
  ec.clear();
  ::ssize_t bytes = ::write(native_handle(),
                            &add,
                            sizeof(add));
  if (bytes < 0) {
    ec.assign(errno,
              std::generic_category());
  } else {
    assert(bytes == sizeof(add));
  }
}

}
