#include <asio_uring/asio/read.hpp>

#include <asio_uring/read.hpp>

namespace asio_uring::asio {

std::size_t read(int fd,
                 boost::asio::mutable_buffer buffer,
                 std::error_code& ec) noexcept
{
  return asio_uring::read(fd,
                          buffer.data(),
                          buffer.size(),
                          ec);
}

}
