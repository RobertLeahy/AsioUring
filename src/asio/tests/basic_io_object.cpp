#include <asio_uring/asio/basic_io_object.hpp>

#include <type_traits>
#include <utility>
#include <asio_uring/asio/execution_context.hpp>
#include <asio_uring/asio/service.hpp>

#include <catch2/catch.hpp>

namespace asio_uring::asio::tests {
namespace {

using io_object = basic_io_object<execution_context,
                                  service>;

static_assert(std::is_nothrow_move_constructible_v<io_object>);
static_assert(std::is_nothrow_move_assignable_v<io_object>);

TEST_CASE("basic_io_object construct") {
  execution_context ctx(10);
  io_object obj(ctx);
  CHECK(obj.get_executor() == ctx.get_executor());
  CHECK(&obj.get_service() == &boost::asio::use_service<service>(ctx));
}

TEST_CASE("basic_io_object move construct") {
  execution_context ctx(10);
  io_object obj(ctx);
  CHECK(obj.get_executor() == ctx.get_executor());
  CHECK(&obj.get_service() == &boost::asio::use_service<service>(ctx));
  io_object moved(std::move(obj));
  CHECK(moved.get_executor() == ctx.get_executor());
  CHECK(&moved.get_service() == &boost::asio::use_service<service>(ctx));
}

TEST_CASE("basic_io_object move assign") {
  execution_context ctx(10);
  io_object first(ctx);
  CHECK(first.get_executor() == ctx.get_executor());
  CHECK(&first.get_service() == &boost::asio::use_service<service>(ctx));
  io_object second(ctx);
  CHECK(second.get_executor() == ctx.get_executor());
  CHECK(&second.get_service() == &boost::asio::use_service<service>(ctx));
  first = std::move(second);
  CHECK(first.get_executor() == ctx.get_executor());
  CHECK(&first.get_service() == &boost::asio::use_service<service>(ctx));
}

}
}
