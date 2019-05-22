/**
 *  \file
 */

#pragma once

#include <system_error>
#include <boost/system/error_code.hpp>

namespace asio_uring::asio {

/**
 *  Attempts to convert a `std::error_code` to
 *  a `boost::system::error_code`.
 *
 *  If the conversion is not possible an
 *  exception is thrown.
 *
 *  \param [in] ec
 *    The `std::error_code` to convert.
 *
 *  \return
 *    The converted `boost::system::error_code`.
 */
boost::system::error_code to_boost_error_code(std::error_code ec);

/**
 *  Derives from `std::error_code`, is
 *  constructible from `std::error_code`,
 *  and implicitly converts to
 *  `boost::system::error_code` via
 *  \ref to_boost_error_code.
 */
class error_code : public std::error_code {
public:
  using std::error_code::error_code;
  /**
   *  Constructs the base sub-object from
   *  a `std::error_code`.
   *
   *  \param [in] ec
   *    The `std::error_code` to copy construct
   *    from.
   */
  explicit error_code(std::error_code ec) noexcept;
  /**
   *  Returns the resul of calling
   *  \ref to_boost_error_code with
   *  `*this`.
   *
   *  \return
   *    The converted `boost::system::error_code`.
   */
  operator boost::system::error_code() const;
};

}
