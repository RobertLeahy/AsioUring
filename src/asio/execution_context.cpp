#include <asio_uring/asio/execution_context.hpp>

namespace asio_uring::asio {

execution_context::~execution_context() noexcept {
  shutdown();
}

}
