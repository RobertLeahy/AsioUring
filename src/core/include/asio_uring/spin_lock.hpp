/**
 *  \file
 */

#pragma once

#include <atomic>

namespace asio_uring {

class spin_lock {
public:
  spin_lock() noexcept;
  spin_lock(const spin_lock&) = delete;
  spin_lock(spin_lock&&) = delete;
  spin_lock& operator=(const spin_lock&) = delete;
  spin_lock& operator=(spin_lock&&) = delete;
  void lock() noexcept;
  void unlock() noexcept;
  bool try_lock() noexcept;
private:
  std::atomic_flag flag_;
};

}
