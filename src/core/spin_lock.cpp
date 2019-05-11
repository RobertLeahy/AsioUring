#include <asio_uring/spin_lock.hpp>

#include <atomic>
#include <cassert>

namespace asio_uring {

spin_lock::spin_lock() noexcept
  : flag_(ATOMIC_FLAG_INIT)
{}

void spin_lock::lock() noexcept {
  while (!try_lock());
}

void spin_lock::unlock() noexcept {
  assert(!try_lock());
  flag_.clear(std::memory_order_release);
}

bool spin_lock::try_lock() noexcept {
  return !flag_.test_and_set(std::memory_order_acquire);
}

}
