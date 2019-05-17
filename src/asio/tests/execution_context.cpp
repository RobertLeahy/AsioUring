#include <asio_uring/asio/execution_context.hpp>

#include <asio_uring/asio/service.hpp>
#include <boost/asio/execution_context.hpp>

#include <catch2/catch.hpp>

namespace asio_uring::asio::tests {
namespace {

TEST_CASE("execution_context use_service",
          "[execution_context]")
{
  execution_context ctx(10);
  auto&& svc = boost::asio::use_service<service>(ctx);
}

}
}
