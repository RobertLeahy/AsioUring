/**
 *  \file
 */

#pragma once

#include <cstdint>
#include <system_error>
#include "fd.hpp"

namespace asio_uring {

/**
 *  Wraps a file descriptor created by a call
 *  to `eventfd`.
 */
class eventfd : public fd {
public:
  /**
   *  The type of integer used to represent the
   *  value of the event count. 64 bit unsigned.
   */
  using integer_type = std::uint64_t;
  /**
   *  Creates a new event file descriptor by
   *  calling `eventfd` with the provided arguments
   *  and checking the returned value (an exception
   *  is thrown on failure).
   *
   *  \param [in] initval
   *    See documentation for `eventfd`.
   *  \param [in] flags
   *    See documentation for `eventfd`.
   */
  explicit eventfd(unsigned int initval = 0,
                   int flags = 0);
  /**
   *  Reads the current value of the event counter.
   *
   *  Throws on error.
   *
   *  \return
   *    The current value of the event counter.
   */
  integer_type read();
  /**
   *  Reads the current value of the event counter.
   *
   *  Does not throw.
   *
   *  \param [out] ec
   *    A `std::error_code` which shall receive the
   *    result of the operation.
   *
   *  \return
   *    The current value of the event counter. If
   *    `ec` is truthy after the return this value
   *    is not meaningful.
   */
  integer_type read(std::error_code& ec) noexcept;
  /**
   *  Adds to the event counter.
   *
   *  Throws on error.
   *
   *  \param [in] add
   *    The amount to add to the event counter.
   */
  void write(integer_type add);
  /**
   *  Adds to the event counter.
   *
   *  Does not throw.
   *
   *  \param [in] add
   *    The amount to add to the event counter.
   *  \param [out] ec
   *    A `std::error_code` which shall receive the
   *    result of the operation.
   */
  void write(integer_type add,
             std::error_code& ec) noexcept;
};

}
