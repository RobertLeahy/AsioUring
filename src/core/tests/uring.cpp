#include <asio_uring/uring.hpp>

#include <algorithm>
#include <cstring>
#include <iterator>
#include <optional>
#include <string>
#include <system_error>
#include <type_traits>
#include <asio_uring/fd.hpp>
#include <asio_uring/liburing.hpp>
#include <boost/endian/conversion.hpp>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <unistd.h>

#include <catch2/catch.hpp>

namespace asio_uring::tests {
namespace {

static_assert(!std::is_default_constructible_v<uring>);
static_assert(!std::is_move_constructible_v<uring>);
static_assert(!std::is_move_assignable_v<uring>);
static_assert(!std::is_copy_constructible_v<uring>);
static_assert(!std::is_copy_assignable_v<uring>);

TEST_CASE("uring",
          "[uring]")
{
  uring ring(1);
  REQUIRE(ring.native_handle());
  CHECK(ring.native_handle()->ring_fd != -1);
}

TEST_CASE("uring no entries",
          "[uring]")
{
  std::optional<std::error_code> ec;
  try {
    uring ring(0);
  } catch (const std::system_error& ex) {
    ec = ex.code();
  }
  REQUIRE(ec);
  CHECK(ec->value() != 0);
  CHECK(&ec->category() == &std::generic_category());
}

TEST_CASE("uring poll read",
          "[uring]")
{
  uring u(10);
  int pipes[2];
  auto result = ::pipe(pipes);
  CHECK(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  auto sqe = ::io_uring_get_sqe(u.native_handle());
  REQUIRE(sqe);
  ::io_uring_prep_poll_add(sqe,
                           pipes[0],
                           POLLIN);
  result = ::io_uring_submit(u.native_handle());
  REQUIRE(result >= 0);
  ::io_uring_cqe* cqe;
  result = ::io_uring_peek_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  REQUIRE_FALSE(cqe);
  char c = 0;
  auto written = ::write(pipes[1],
                         &c,
                         sizeof(c));
  REQUIRE(written == 1);
  result = ::io_uring_peek_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  REQUIRE(cqe);
  CHECK(cqe->res >= 0);
}

TEST_CASE("uring poll write",
          "[uring]")
{
  uring u(10);
  int pipes[2];
  auto result = ::pipe(pipes);
  CHECK(result == 0);
  fd read(pipes[0]);
  fd write(pipes[1]);
  auto sqe = ::io_uring_get_sqe(u.native_handle());
  REQUIRE(sqe);
  ::io_uring_prep_poll_add(sqe,
                           pipes[1],
                           POLLOUT);
  result = ::io_uring_submit(u.native_handle());
  REQUIRE(result >= 0);
  ::io_uring_cqe* cqe;
  result = ::io_uring_peek_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  REQUIRE(cqe);
  CHECK(cqe->res >= 0);
}

TEST_CASE("uring file read",
          "[uring]")
{
  std::string str("hello");
  char filename[] = "/tmp/XXXXXX";
  fd file(::mkstemp(filename));
  INFO("Temporary file is " << filename);
  auto written = ::write(file.native_handle(),
                         str.data(),
                         str.size());
  REQUIRE(written == str.size());
  file = fd();
  file = fd(::open(filename,
                   O_RDONLY));
  char buffer[5];
  ::iovec iov;
  iov.iov_base = buffer;
  iov.iov_len = sizeof(buffer);
  uring u(10);
  auto sqe = ::io_uring_get_sqe(u.native_handle());
  REQUIRE(sqe);
  ::io_uring_prep_readv(sqe,
                        file.native_handle(),
                        &iov,
                        1,
                        0);
  auto result = ::io_uring_submit(u.native_handle());
  REQUIRE(result >= 0);
  ::io_uring_cqe* cqe;
  result = ::io_uring_wait_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  REQUIRE(cqe);
  REQUIRE(cqe->res == sizeof(buffer));
  ::io_uring_cqe_seen(u.native_handle(),
                      cqe);
  CHECK(std::equal(str.begin(),
                   str.end(),
                   std::begin(buffer),
                   std::end(buffer)));
  sqe = ::io_uring_get_sqe(u.native_handle());
  REQUIRE(sqe);
  ::io_uring_prep_readv(sqe,
                        file.native_handle(),
                        &iov,
                        1,
                        sizeof(buffer));
  result = ::io_uring_submit(u.native_handle());
  REQUIRE(result >= 0);
  result = ::io_uring_wait_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  REQUIRE(cqe);
  CHECK(cqe->res == 0);
}

TEST_CASE("uring file write",
          "[uring]")
{
  std::string str("hello");
  char filename[] = "/tmp/XXXXXX";
  fd file(::mkstemp(filename));
  INFO("Temporary file is " << filename);
  ::iovec iov;
  iov.iov_base = str.data();
  iov.iov_len = str.size();
  uring u(10);
  auto sqe = ::io_uring_get_sqe(u.native_handle());
  REQUIRE(sqe);
  ::io_uring_prep_writev(sqe,
                         file.native_handle(),
                         &iov,
                         1,
                         0);
  auto result = ::io_uring_submit(u.native_handle());
  REQUIRE(result >= 0);
  ::io_uring_cqe* cqe;
  result = ::io_uring_wait_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  REQUIRE(cqe);
  REQUIRE(cqe->res == str.size());
  ::io_uring_cqe_seen(u.native_handle(),
                      cqe);
  std::string second("o world");
  iov.iov_base = second.data();
  iov.iov_len = second.size();
  sqe = ::io_uring_get_sqe(u.native_handle());
  ::io_uring_prep_writev(sqe,
                         file.native_handle(),
                         &iov,
                         1,
                         4);
  result = ::io_uring_submit(u.native_handle());
  REQUIRE(result >= 0);
  result = ::io_uring_wait_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  REQUIRE(cqe);
  REQUIRE(cqe->res == second.size());
  file = fd();
  file = fd(::open(filename,
                   O_RDONLY));
  char buffer[5];
  auto read = ::read(file.native_handle(),
                     buffer,
                     sizeof(buffer));
  REQUIRE(read == sizeof(buffer));
  CHECK(std::equal(str.begin(),
                   str.end(),
                   std::begin(buffer),
                   std::end(buffer)));
  char buffer2[6];
  read = ::read(file.native_handle(),
                buffer2,
                sizeof(buffer2));
  REQUIRE(read == sizeof(buffer2));
  CHECK(std::equal(second.begin() + 1,
                   second.end(),
                   std::begin(buffer2),
                   std::end(buffer2)));
  char c;
  read = ::read(file.native_handle(),
                &c,
                sizeof(c));
  CHECK(read == 0);
}

TEST_CASE("uring tcp socket poll read",
          "[uring]")
{
  fd listen(::socket(AF_INET,
                     SOCK_STREAM,
                     0));
  ::sockaddr_in addr;
  std::memset(&addr,
              0,
              sizeof(addr));
  addr.sin_family = AF_INET;
  uint32_t ip = 127;
  ip <<= 8;
  ip <<= 8;
  ip <<= 8;
  ip |= 1;
  boost::endian::native_to_big_inplace(ip);
  addr.sin_addr.s_addr = ip;
  auto result = ::bind(listen.native_handle(),
                       reinterpret_cast<const ::sockaddr*>(&addr),
                       sizeof(addr));
  REQUIRE(result == 0);
  ::socklen_t addr_len = sizeof(addr);
  result = ::getsockname(listen.native_handle(),
                         reinterpret_cast<::sockaddr*>(&addr),
                         &addr_len);
  REQUIRE(result == 0);
  REQUIRE(addr.sin_port != 0);
  result = ::listen(listen.native_handle(),
                    1);
  REQUIRE(result == 0);
  fd connect(::socket(AF_INET,
                      SOCK_STREAM,
                      0));
  result = ::connect(connect.native_handle(),
                     reinterpret_cast<const ::sockaddr*>(&addr),
                     sizeof(addr));
  REQUIRE(result == 0);
  addr_len = sizeof(addr);
  fd accepted(::accept(listen.native_handle(),
                       reinterpret_cast<::sockaddr*>(&addr),
                       &addr_len));
  listen = fd();
  uring u(10);
  auto sqe = ::io_uring_get_sqe(u.native_handle());
  REQUIRE(sqe);
  ::io_uring_prep_poll_add(sqe,
                           accepted.native_handle(),
                           POLLIN);
  result = ::io_uring_submit(u.native_handle());
  REQUIRE(result >= 0);
  ::io_uring_cqe* cqe;
  result = ::io_uring_peek_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  REQUIRE_FALSE(cqe);
  char c = 'A';
  auto bytes = ::write(connect.native_handle(),
                       &c,
                       sizeof(c));
  REQUIRE(bytes == sizeof(c));
  result = ::io_uring_wait_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  REQUIRE(cqe);
  CHECK(cqe->res > 0);
  char out;
  bytes = ::read(accepted.native_handle(),
                 &out,
                 sizeof(out));
  REQUIRE(bytes == sizeof(out));
  CHECK(out == 'A');
}

TEST_CASE("uring tcp socket poll write",
          "[uring]")
{
  fd listen(::socket(AF_INET,
                     SOCK_STREAM,
                     0));
  ::sockaddr_in addr;
  std::memset(&addr,
              0,
              sizeof(addr));
  addr.sin_family = AF_INET;
  uint32_t ip = 127;
  ip <<= 8;
  ip <<= 8;
  ip <<= 8;
  ip |= 1;
  boost::endian::native_to_big_inplace(ip);
  addr.sin_addr.s_addr = ip;
  auto result = ::bind(listen.native_handle(),
                       reinterpret_cast<const ::sockaddr*>(&addr),
                       sizeof(addr));
  REQUIRE(result == 0);
  ::socklen_t addr_len = sizeof(addr);
  result = ::getsockname(listen.native_handle(),
                         reinterpret_cast<::sockaddr*>(&addr),
                         &addr_len);
  REQUIRE(result == 0);
  REQUIRE(addr.sin_port != 0);
  result = ::listen(listen.native_handle(),
                    1);
  REQUIRE(result == 0);
  fd connect(::socket(AF_INET,
                      SOCK_STREAM,
                      0));
  result = ::connect(connect.native_handle(),
                     reinterpret_cast<const ::sockaddr*>(&addr),
                     sizeof(addr));
  REQUIRE(result == 0);
  addr_len = sizeof(addr);
  fd accepted(::accept(listen.native_handle(),
                       reinterpret_cast<::sockaddr*>(&addr),
                       &addr_len));
  listen = fd();
  uring u(10);
  auto sqe = ::io_uring_get_sqe(u.native_handle());
  REQUIRE(sqe);
  ::io_uring_prep_poll_add(sqe,
                           accepted.native_handle(),
                           POLLOUT);
  result = ::io_uring_submit(u.native_handle());
  REQUIRE(result >= 0);
  ::io_uring_cqe* cqe;
  result = ::io_uring_peek_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  REQUIRE(cqe);
  CHECK(cqe->res > 0);
}

TEST_CASE("uring tcp socket poll read then remove",
          "[uring]")
{
  fd listen(::socket(AF_INET,
                     SOCK_STREAM,
                     0));
  ::sockaddr_in addr;
  std::memset(&addr,
              0,
              sizeof(addr));
  addr.sin_family = AF_INET;
  uint32_t ip = 127;
  ip <<= 8;
  ip <<= 8;
  ip <<= 8;
  ip |= 1;
  boost::endian::native_to_big_inplace(ip);
  addr.sin_addr.s_addr = ip;
  auto result = ::bind(listen.native_handle(),
                       reinterpret_cast<const ::sockaddr*>(&addr),
                       sizeof(addr));
  REQUIRE(result == 0);
  ::socklen_t addr_len = sizeof(addr);
  result = ::getsockname(listen.native_handle(),
                         reinterpret_cast<::sockaddr*>(&addr),
                         &addr_len);
  REQUIRE(result == 0);
  REQUIRE(addr.sin_port != 0);
  result = ::listen(listen.native_handle(),
                    1);
  REQUIRE(result == 0);
  fd connect(::socket(AF_INET,
                      SOCK_STREAM,
                      0));
  result = ::connect(connect.native_handle(),
                     reinterpret_cast<const ::sockaddr*>(&addr),
                     sizeof(addr));
  REQUIRE(result == 0);
  addr_len = sizeof(addr);
  fd accepted(::accept(listen.native_handle(),
                       reinterpret_cast<::sockaddr*>(&addr),
                       &addr_len));
  listen = fd();
  uring u(10);
  auto sqe = ::io_uring_get_sqe(u.native_handle());
  REQUIRE(sqe);
  ::io_uring_prep_poll_add(sqe,
                           accepted.native_handle(),
                           POLLIN);
  int is[] = {1, 2};
  ::io_uring_sqe_set_data(sqe,
                          is);
  result = ::io_uring_submit(u.native_handle());
  REQUIRE(result >= 0);
  ::io_uring_cqe* cqe;
  result = ::io_uring_peek_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  REQUIRE_FALSE(cqe);
  sqe = ::io_uring_get_sqe(u.native_handle());
  REQUIRE(sqe);
  ::io_uring_prep_poll_remove(sqe,
                              is);
  ::io_uring_sqe_set_data(sqe,
                          &is[1]);
  result = ::io_uring_submit(u.native_handle());
  REQUIRE(result >= 0);
  result = ::io_uring_peek_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  REQUIRE(cqe);
  CHECK(cqe->res == 0);
  auto ptr = static_cast<int*>(::io_uring_cqe_get_data(cqe));
  CHECK(&is[1] == ptr);
  ::io_uring_cqe_seen(u.native_handle(),
                      cqe);
  result = ::io_uring_peek_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  REQUIRE(cqe);
  CHECK(::io_uring_cqe_get_data(cqe) == is);
  CHECK(cqe->res == 0);
}

TEST_CASE("uring remove poll does not exist",
          "[uring]")
{
  uring u(10);
  auto sqe = ::io_uring_get_sqe(u.native_handle());
  REQUIRE(sqe);
  ::io_uring_prep_poll_remove(sqe,
                              nullptr);
  auto result = ::io_uring_submit(u.native_handle());
  REQUIRE(result >= 0);
  ::io_uring_cqe* cqe;
  result = ::io_uring_peek_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  REQUIRE(cqe);
  CHECK(cqe->res == -ENOENT);
}

TEST_CASE("uring nop",
          "[uring]")
{
  uring u(10);
  auto sqe = ::io_uring_get_sqe(u.native_handle());
  REQUIRE(sqe);
  ::io_uring_prep_nop(sqe);
  int i = 5;
  ::io_uring_sqe_set_data(sqe,
                          &i);
  auto result = ::io_uring_submit(u.native_handle());
  REQUIRE(result >= 0);
  ::io_uring_cqe* cqe;
  result = ::io_uring_peek_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  REQUIRE(cqe);
  CHECK(cqe->res == 0);
  auto* ptr = static_cast<int*>(::io_uring_cqe_get_data(cqe));
  CHECK(ptr == &i);
}

TEST_CASE("uring tcp socket poll read then close",
          "[uring]")
{
  fd listen(::socket(AF_INET,
                     SOCK_STREAM,
                     0));
  ::sockaddr_in addr;
  std::memset(&addr,
              0,
              sizeof(addr));
  addr.sin_family = AF_INET;
  uint32_t ip = 127;
  ip <<= 8;
  ip <<= 8;
  ip <<= 8;
  ip |= 1;
  boost::endian::native_to_big_inplace(ip);
  addr.sin_addr.s_addr = ip;
  auto result = ::bind(listen.native_handle(),
                       reinterpret_cast<const ::sockaddr*>(&addr),
                       sizeof(addr));
  REQUIRE(result == 0);
  ::socklen_t addr_len = sizeof(addr);
  result = ::getsockname(listen.native_handle(),
                         reinterpret_cast<::sockaddr*>(&addr),
                         &addr_len);
  REQUIRE(result == 0);
  REQUIRE(addr.sin_port != 0);
  result = ::listen(listen.native_handle(),
                    1);
  REQUIRE(result == 0);
  fd connect(::socket(AF_INET,
                      SOCK_STREAM,
                      0));
  result = ::connect(connect.native_handle(),
                     reinterpret_cast<const ::sockaddr*>(&addr),
                     sizeof(addr));
  REQUIRE(result == 0);
  addr_len = sizeof(addr);
  fd accepted(::accept(listen.native_handle(),
                       reinterpret_cast<::sockaddr*>(&addr),
                       &addr_len));
  listen = fd();
  uring u(10);
  auto sqe = ::io_uring_get_sqe(u.native_handle());
  REQUIRE(sqe);
  ::io_uring_prep_poll_add(sqe,
                           accepted.native_handle(),
                           POLLIN);
  result = ::io_uring_submit(u.native_handle());
  REQUIRE(result >= 0);
  ::io_uring_cqe* cqe;
  result = ::io_uring_peek_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  REQUIRE_FALSE(cqe);
  accepted = fd();
  result = ::io_uring_peek_cqe(u.native_handle(),
                               &cqe);
  REQUIRE(result >= 0);
  //  I'd have expected the operation to fail,
  //  but apparently it vanishes into a black
  //  hole never to complete
  CHECK_FALSE(cqe);
}

}
}
