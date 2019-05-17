#include <asio_uring/asio/iovec.hpp>

#include <boost/asio/buffer.hpp>
#include <sys/uio.h>

#include <catch2/catch.hpp>

namespace asio_uring::asio::tests {
namespace {

TEST_CASE("to_iovec empty mutable_buffer") {
  boost::asio::mutable_buffer buffer;
  auto iov = to_iovec(buffer);
  CHECK(iov.iov_len == 0);
}

TEST_CASE("to_iovec empty const_buffer") {
  boost::asio::const_buffer buffer;
  auto iov = to_iovec(buffer);
  CHECK(iov.iov_len == 0);
}

TEST_CASE("to_iovec mutable_buffer") {
  int i;
  boost::asio::mutable_buffer mb(&i,
                                 sizeof(i));
  auto iov = to_iovec(mb);
  CHECK(iov.iov_base == &i);
  CHECK(iov.iov_len == sizeof(i));
}

TEST_CASE("to_iovec const_buffer") {
  const int i = 0;
  boost::asio::const_buffer cb(&i,
                               sizeof(i));
  auto iov = to_iovec(cb);
  CHECK(iov.iov_base == &i);
  CHECK(iov.iov_len == sizeof(i));
}

TEST_CASE("to_iovecs mutable") {
  int i;
  boost::asio::mutable_buffer mb(&i,
                                 sizeof(i));
  ::iovec iov;
  to_iovecs(mb,
            &iov);
  CHECK(iov.iov_base == &i);
  CHECK(iov.iov_len == sizeof(i));
}

TEST_CASE("to_iovecs const") {
  const int i = 0;
  boost::asio::const_buffer cb(&i,
                               sizeof(i));
  ::iovec iov;
  to_iovecs(cb,
            &iov);
  CHECK(iov.iov_base == &i);
  CHECK(iov.iov_len == sizeof(i));
}

}
}
