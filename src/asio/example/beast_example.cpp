#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <asio_uring/asio/connect_file.hpp>
#include <asio_uring/asio/execution_context.hpp>
#include <asio_uring/fd.hpp>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/endian/conversion.hpp>

namespace {

class freeaddrinfo_guard : private boost::noncopyable {
public:
  explicit freeaddrinfo_guard(::addrinfo* addrs) noexcept
    : addrs_(addrs)
  {}
  ~freeaddrinfo_guard() noexcept {
    ::freeaddrinfo(addrs_);
  }
private:
  ::addrinfo* addrs_;
};

void Main(int argc,
          char** argv)
{
  if (argc != 2) {
    throw std::runtime_error("Incorrect number of command line arguments");
  }
  std::string host(argv[1]);
  ::addrinfo* out;
  int result = ::getaddrinfo(host.c_str(),
                             nullptr,
                             nullptr,
                             &out);
  if (result != 0) {
    throw std::runtime_error(::gai_strerror(result));
  }
  freeaddrinfo_guard g(out);
  for (; out; out = out->ai_next) {
    if ((out->ai_family == AF_INET) ||
        (out->ai_family == AF_INET6))
    {
      break;
    }
  }
  if (!out) {
    throw std::runtime_error("No suitable address from resolution");
  }
  asio_uring::fd socket(::socket(out->ai_family,
                                 SOCK_STREAM | SOCK_NONBLOCK,
                                 0));
  std::error_code ec;
  auto redirect = [&](auto e,
                      auto&&...) noexcept
                  {
                    ec = e;
                  };
  asio_uring::asio::execution_context ctx(100);
  asio_uring::asio::connect_file connect(ctx,
                                         std::move(socket));
  if (out->ai_family == AF_INET) {
    auto&& addr = *reinterpret_cast<::sockaddr_in*>(out->ai_addr);
    addr.sin_port = 80;
    boost::endian::native_to_big_inplace(addr.sin_port);
    connect.async_connect(addr,
                          redirect);
  } else {
    auto&& addr = *reinterpret_cast<::sockaddr_in6*>(out->ai_addr);
    addr.sin6_port = 80;
    boost::endian::native_to_big_inplace(addr.sin6_port);
    connect.async_connect(addr,
                          redirect);
  }
  auto handlers = ctx.run();
  if (ec) {
    throw std::system_error(ec);
  }
  boost::beast::http::request<boost::beast::http::empty_body> request;
  request.method(boost::beast::http::verb::get);
  request.target("/");
  boost::beast::http::async_write(connect,
                                  request,
                                  redirect);
  ctx.restart();
  handlers += ctx.run();
  if (ec) {
    throw std::system_error(ec);
  }
  boost::beast::flat_buffer dynamic_buffer;
  boost::beast::http::response<boost::beast::http::string_body> response;
  boost::beast::http::async_read(connect,
                                 dynamic_buffer,
                                 response,
                                 redirect);
  ctx.restart();
  handlers += ctx.run();
  if (ec) {
    throw std::system_error(ec);
  }
  std::cout << response.result_int() << ' ' << response.reason() << '\n';
  for (auto&& field : response.base()) {
    std::cout << field.name_string() << ": " << field.value() << '\n';
  }
  std::cout << response.body() << "\n"
               "Done! Ran " << handlers << " handlers" << std::endl;
}

}

int main(int argc,
         char** argv)
{
  try {
    try {
      Main(argc,
           argv);
    } catch (const std::exception& ex) {
      std::cerr << "ERROR: " << ex.what() << std::endl;
      throw;
    } catch (...) {
      std::cerr << "ERROR" << std::endl;
      throw;
    }
  } catch (...) {
    return EXIT_FAILURE;
  }
}
