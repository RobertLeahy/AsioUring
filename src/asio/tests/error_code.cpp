#include <asio_uring/asio/error_code.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <boost/system/error_code.hpp>
#include <errno.h>

#include <catch2/catch.hpp>

namespace asio_uring::asio::tests {
namespace {

TEST_CASE("to_boost_error_code default constructed",
          "[to_boost_error_code][error_code]")
{
  std::error_code ec;
  auto bec = to_boost_error_code(ec);
  CHECK(bec.message() == ec.message());
  CHECK(bec.value() == ec.value());
  CHECK(bec == boost::system::error_code());
}

TEST_CASE("to_boost_error_code std::generic_category",
          "[to_boost_error_code][error_code]")
{
  std::error_code ec(EBADF,
                     std::generic_category());
  auto bec = to_boost_error_code(ec);
  CHECK(bec.message() == ec.message());
  CHECK(bec.value() == ec.value());
}

class dummy_category : public std::error_category {
public:
  virtual const char* name() const noexcept override {
    return "dummy";
  }
  virtual std::string message(int) const override {
    return "dummy";
  }
};

TEST_CASE("to_boost_error_code not built in category",
          "[to_boost_error_code][error_code]")
{
  dummy_category category;
  std::error_code ec(1,
                     category);
  std::optional<std::string> str;
  try {
    to_boost_error_code(ec);
  } catch (const std::runtime_error& ex) {
    str.emplace(ex.what());
  }
  REQUIRE(str);
  CHECK(str == "Cannot convert std::error_category dummy to boost::system::error_category");
}

TEST_CASE("error_code default constructed",
          "[error_code]")
{
  std::error_code ec;
  error_code e(ec);
  CHECK(e == ec);
  boost::system::error_code bec(e);
  CHECK(bec.message() == ec.message());
  CHECK(bec.value() == ec.value());
  CHECK(bec == boost::system::error_code());
}

TEST_CASE("error_code std::generic_category",
          "[error_code]")
{
  std::error_code ec(EBADF,
                     std::generic_category());
  error_code e(ec);
  CHECK(e == ec);
  boost::system::error_code bec(e);
  CHECK(bec.message() == ec.message());
  CHECK(bec.value() == ec.value());
}

TEST_CASE("error_code not built in category",
          "[error_code]")
{
  dummy_category category;
  std::error_code ec(1,
                     category);
  error_code e(ec);
  std::optional<std::string> str;
  try {
    boost::system::error_code bec(e);
  } catch (const std::runtime_error& ex) {
    str.emplace(ex.what());
  }
  REQUIRE(str);
  CHECK(str == "Cannot convert std::error_category dummy to boost::system::error_category");
}

}
}
