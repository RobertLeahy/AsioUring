#include <asio_uring/asio/write.hpp>

#include <system_error>
#include <asio_uring/asio/error_code.hpp>
#include <asio_uring/write.hpp>

namespace asio_uring::asio {

std::size_t write(int fd,
                  boost::asio::const_buffer buffer,
                  boost::system::error_code& ec) noexcept
{
  ec.clear();
  std::error_code sec;
  auto retr = asio_uring::write(fd,
                                buffer.data(),
                                buffer.size(),
                                sec);
  if (sec) {
    ec = to_boost_error_code(sec);
  }
  return retr;
}

}
