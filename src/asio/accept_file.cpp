#include <asio_uring/asio/accept_file.hpp>

#include <cassert>
#include <memory>
#include <utility>

namespace asio_uring::asio {

accept_file::accept_file(execution_context& ctx,
                         fd file)
  : impl_(std::make_shared<impl>(ctx,
                                 std::move(file)))
{}

accept_file::executor_type accept_file::get_executor() const noexcept {
  assert(impl_);
  return impl_->get_executor();
}

accept_file::implementation_type& accept_file::get_implementation() noexcept {
  assert(impl_);
  return impl_->get_implementation();
}

const accept_file::implementation_type& accept_file::get_implementation() const noexcept {
  assert(impl_);
  return impl_->get_implementation();
}

accept_file::service_type& accept_file::get_service() noexcept {
  assert(impl_);
  return impl_->get_service();
}

}
