/**
 *  \file
 */

#pragma once

#include <atomic>

namespace asio_uring {

/**
 *  Models `Lockable` but never blocks (instead
 *  spinning on a `std::atomic_flag`).
 */
class spin_lock {
public:
  /**
   *  Constructs a spin_lock which is not locked.
   */
  spin_lock() noexcept;
  spin_lock(const spin_lock&) = delete;
  spin_lock(spin_lock&&) = delete;
  spin_lock& operator=(const spin_lock&) = delete;
  spin_lock& operator=(spin_lock&&) = delete;
  /**
   *  Acquires the lock.
   *
   *  Calling this from a thread which already holds
   *  the lock results in a deadlock.
   */
  void lock() noexcept;
  /**
   *  Releases the lock.
   *
   *  Calling this from a thread which does not
   *  hold the lock results in undefined behavior.
   */
  void unlock() noexcept;
  /**
   *  Attempts to acquire the lock through a
   *  single call to `std::atomic_flag::test_and_set`.
   *
   *  \return
   *    `true` if the lock was acquired (and the caller
   *    is now obligated to call \ref unlock at some
   *    point in the future), `false` otherwise (in
   *    which case the caller must not call \ref unlock).
   */
  bool try_lock() noexcept;
private:
  std::atomic_flag flag_;
};

}
