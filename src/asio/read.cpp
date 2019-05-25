#include <asio_uring/asio/read.hpp>

#include <system_error>
#include <asio_uring/asio/error_code.hpp>
#include <asio_uring/read.hpp>

namespace asio_uring::asio {

std::size_t read(int fd,
                 boost::asio::mutable_buffer buffer,
                 boost::system::error_code& ec) noexcept
{
  ec.clear();
  std::error_code sec;
  auto retr = asio_uring::read(fd,
                               buffer.data(),
                               buffer.size(),
                               sec);
  if (sec) {
    ec = to_boost_error_code(sec);
  }
  return retr;
}

}
