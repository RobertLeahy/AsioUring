/**
 *  \file
 */

#pragma once

namespace asio_uring {

class fd {
public:
  using const_native_handle_type = int;
  using native_handle_type = const_native_handle_type;
  fd(const fd&) = delete;
  fd& operator=(const fd&) = delete;
  fd() noexcept;
  explicit fd(const_native_handle_type handle);
  fd(fd&&) noexcept;
  fd& operator=(fd&&) noexcept;
  ~fd() noexcept;
  const_native_handle_type native_handle() const noexcept;
private:
  const_native_handle_type handle_;
};

}
