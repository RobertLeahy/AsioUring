/**
 *  \file
 */

#pragma once

#include "liburing.hpp"

namespace asio_uring {

class uring {
public:
  using native_handle_type = ::io_uring*;
  using const_native_handle_type = const ::io_uring*;
  uring() = delete;
  uring(const uring&) = delete;
  uring(uring&&) = delete;
  uring& operator=(const uring&) = delete;
  uring& operator=(uring&&) = delete;
  explicit uring(unsigned entries,
                 unsigned flags = 0);
  ~uring() noexcept;
  native_handle_type native_handle() noexcept;
  const_native_handle_type native_handle() const noexcept;
private:
  ::io_uring ring_;
};

}
