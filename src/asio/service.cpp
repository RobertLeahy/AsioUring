#include <asio_uring/asio/service.hpp>

#include <asio_uring/asio/execution_context.hpp>
#include <asio_uring/execution_context.hpp>
#include <boost/asio/error.hpp>
#include <boost/system/error_code.hpp>

namespace asio_uring::asio {

service::rw_result_type service::to_rw_result(int res) noexcept {
  rw_result_type retr(boost::system::error_code(),
                      0);
  if (res < 0) {
    retr.first.assign(-res,
                      boost::system::generic_category());
  } else {
    retr.second = res;
  }
  return retr;
}

boost::system::error_code service::to_poll_add_result(int res) noexcept {
  if (res > 0) {
    return boost::system::error_code();
  }
  if (res < 0) {
    return boost::system::error_code(-res,
                                     boost::system::generic_category());
  }
  return make_error_code(boost::asio::error::operation_aborted);
}

boost::system::error_code service::to_poll_remove_result(int res) noexcept {
  if (res >= 0) {
    return boost::system::error_code();
  }
  return boost::system::error_code(-res,
                                   boost::system::generic_category());
}

boost::system::error_code service::to_fsync_result(int res) noexcept {
  return to_poll_remove_result(res);
}

service::service(boost::asio::execution_context& ctx)
  : asio_uring::service                    (static_cast<asio_uring::asio::execution_context&>(ctx)),
    boost::asio::execution_context::service(ctx)
{}

execution_context& service::context() const noexcept {
  return static_cast<execution_context&>(asio_uring::service::context());
}

void service::shutdown() noexcept {
  asio_uring::service::shutdown();
}

}
