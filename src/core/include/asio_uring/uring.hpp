/**
 *  \file
 */

#pragma once

#include "liburing.hpp"

namespace asio_uring {

/**
 *  An owning wrapper around a `::io_uring`.
 */
class uring {
public:
  /**
   *  A type alias for `::io_uring*`.
   */
  using native_handle_type = ::io_uring*;
  /**
   *  A type alias for `const ::io_uring*`.
   */
  using const_native_handle_type = const ::io_uring*;
  uring() = delete;
  uring(const uring&) = delete;
  uring(uring&&) = delete;
  uring& operator=(const uring&) = delete;
  uring& operator=(uring&&) = delete;
  /**
   *  Calls `::io_uring_queue_init` with the
   *  provided arguments and throws `std::system_error`
   *  wrapping the error if that fails.
   *
   *  \param [in] entries
   *    See documentation for `::io_uring_queue_init`.
   *  \param [in] flags
   *    See documentation for `::io_uring_queue_init`.
   *    Defaults to `0`.
   */
  explicit uring(unsigned entries,
                 unsigned flags = 0);
  /**
   *  Calls `::io_uring_queue_exit`.
   */
  ~uring() noexcept;
  /**
   *  @{
   *  Retrieves a pointer to the managed `::io_uring`.
   *
   *  \return
   *    A pointer to the managed `::io_uring`.
   */
  native_handle_type native_handle() noexcept;
  const_native_handle_type native_handle() const noexcept;
  /**
   *  @}
   */
private:
  ::io_uring ring_;
};

}
