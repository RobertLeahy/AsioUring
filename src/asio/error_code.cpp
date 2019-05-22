#include <asio_uring/asio/error_code.hpp>

#include <sstream>
#include <stdexcept>
#include <system_error>
#include <boost/system/error_code.hpp>

namespace asio_uring::asio {

boost::system::error_code to_boost_error_code(std::error_code ec) {
  if (&ec.category() == &std::generic_category()) {
    return boost::system::error_code(ec.value(),
                                     boost::system::generic_category());
  }
  if (&ec.category() == &std::system_category()) {
    return boost::system::error_code(ec.value(),
                                     boost::system::system_category());
  }
  std::ostringstream ss;
  ss << "Cannot convert std::error_category "
     << ec.category().name()
     << " to boost::system::error_category";
  throw std::runtime_error(ss.str());
}

error_code::error_code(std::error_code ec) noexcept
  : std::error_code(ec)
{}

error_code::operator boost::system::error_code() const {
  return to_boost_error_code(*this);
}

}
