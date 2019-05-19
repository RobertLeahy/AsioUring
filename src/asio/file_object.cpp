#include <asio_uring/asio/file_object.hpp>

#include <cassert>
#include <memory>
#include <asio_uring/fd.hpp>

namespace asio_uring::asio {

file_object::file_object(execution_context& ctx,
                         fd file)
  : base(ctx),
    fd_ (std::make_shared<fd>(std::move(file)))
{}

file_object::native_handle_type file_object::native_handle() noexcept {
  assert(fd_);
  return fd_->native_handle();
}

file_object::const_native_handle_type file_object::native_handle() const noexcept {
  assert(fd_);
  return fd_->native_handle();
}

void file_object::reset() noexcept {
  fd_.reset();
}

long file_object::outstanding() const noexcept {
  auto u = fd_.use_count();
  assert(u);
  return u - 1;
}

}
