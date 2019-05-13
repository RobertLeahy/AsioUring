#include <asio_uring/callable_storage.hpp>

#include <cassert>

namespace asio_uring::detail::callable_storage {

base::~base() noexcept {}

indirect::~indirect() noexcept {
  if (inner_) {
    inner_->destroy();
  }
}

void indirect::invoke() {
  auto ptr = inner_;
  assert(ptr);
  inner_ = nullptr;
  ptr->invoke();
}

}
