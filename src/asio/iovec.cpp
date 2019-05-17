#include <asio_uring/asio/iovec.hpp>

#include <cstring>
#include <sys/uio.h>

namespace asio_uring::asio {

namespace {

template<typename Buffer>
::iovec init_iovec(Buffer b) noexcept {
  ::iovec retr;
  std::memset(&retr,
              0,
              sizeof(retr));
  retr.iov_len = b.size();
  return retr;
}

}

::iovec to_iovec(boost::asio::const_buffer cb) noexcept {
  auto retr = init_iovec(cb);
  retr.iov_base = const_cast<void*>(cb.data());
  return retr;
}

::iovec to_iovec(boost::asio::mutable_buffer mb) noexcept {
  auto retr = init_iovec(mb);
  retr.iov_base = mb.data();
  return retr;
}

}
