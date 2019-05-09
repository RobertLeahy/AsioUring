/**
 *  \file
 */

#pragma once

#include <cstdint>
#include <system_error>
#include "fd.hpp"

namespace asio_uring {

class eventfd : public fd {
public:
  using integer_type = std::uint64_t;
  explicit eventfd(unsigned int initval = 0,
                   int flags = 0);
  integer_type read();
  integer_type read(std::error_code& ec) noexcept;
  void write(integer_type add);
  void write(integer_type add,
             std::error_code& ec) noexcept;
};

}
