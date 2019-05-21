/**
 *  \file
 */

#pragma once

namespace asio_uring {

/**
 *  An owning wrapper for a file
 *  descriptor.
 */
class fd {
public:
  /**
   *  @{
   *  A type alias for `int`.
   */
  using const_native_handle_type = int;
  using native_handle_type = const_native_handle_type;
  /**
   *  @}
   */
  fd(const fd&) = delete;
  fd& operator=(const fd&) = delete;
  /**
   *  Wraps an invalid file descriptor (i.e.
   *  -1).
   */
  fd() noexcept;
  /**
   *  Wraps a certain file descriptor.
   *
   *  If the provided file descriptor is invalid
   *  a `std::system_error` wrapping the current
   *  value of `errno` will be thrown.
   *
   *  \param [in] handle
   *    The file descriptor.
   */
  explicit fd(const_native_handle_type handle);
  /**
   *  Creates a file descriptor wrapper which
   *  assumes ownership of the file descriptor
   *  owned by another object, leaving that object
   *  without ownership of the file descriptor.
   *
   *  \param [in] other
   *    The object from which to transfer ownership.
   */
  fd(fd&& other) noexcept;
  /**
   *  Performs the following actions:
   *
   *  - Calls `close` on the wrapped file descriptor
   *    (unless the wrapped file descriptor is invalid)
   *  - Assumes ownership of the file descriptor owned
   *    by `rhs`
   *
   *  \param [in] rhs
   *    The object from which to transfer ownership.
   *
   *  \return
   *    A reference to this object.
   */
  fd& operator=(fd&& rhs) noexcept;
  /**
   *  Calls `close` on the wrapped file descriptor
   *  unless the wrapped file descriptor is invalid.
   */
  ~fd() noexcept;
  /**
   *  Retrieves the wrapped file descriptor.
   *
   *  \return
   *    The wrapped file descripctor.
   */
  const_native_handle_type native_handle() const noexcept;
private:
  const_native_handle_type handle_;
};

}
