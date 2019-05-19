#include <asio_uring/asio/write.hpp>

#include <asio_uring/write.hpp>

namespace asio_uring::asio {

std::size_t write(int fd,
                  boost::asio::const_buffer buffer,
                  std::error_code& ec) noexcept
{
  return asio_uring::write(fd,
                           buffer.data(),
                           buffer.size(),
                           ec);
}

}
