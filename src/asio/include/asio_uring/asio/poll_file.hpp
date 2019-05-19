/**
 *  \file
 */

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <system_error>
#include <type_traits>
#include <utility>
#include <asio_uring/fd.hpp>
#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/post.hpp>
#include "execution_context.hpp"
#include "file_object.hpp"
#include "read.hpp"
#include "write.hpp"
#include <sys/poll.h>

namespace asio_uring::asio {

namespace detail {

template<typename CompletionHandler,
         typename Invoker>
class poll_file_op {
public:
  poll_file_op(Invoker i,
               CompletionHandler h) noexcept(std::is_nothrow_move_constructible_v<CompletionHandler> &&
                                             std::is_nothrow_move_constructible_v<Invoker>)
    : i_(std::move(i)),
      h_(std::move(h))
  {}
  void operator()(std::error_code ec) {
    i_(ec,
       std::move(h_));
  }
  const CompletionHandler& completion_handler() const noexcept {
    return h_;
  }
private:
  Invoker           i_;
  CompletionHandler h_;
};

}

class poll_file : public file_object {
private:
  using signature = void(std::error_code,
                         std::size_t);
public:
  poll_file(execution_context& ctx,
            fd file);
  template<typename CompletionToken>
  auto async_poll_in(CompletionToken&& token) {
    return get_service().initiate_poll_add(get_implementation(),
                                           native_handle(),
                                           POLLIN,
                                           wrap_token(std::forward<CompletionToken>(token)));
  }
  template<typename CompletionToken>
  auto async_poll_out(CompletionToken&& token) {
    return get_service().initiate_poll_add(get_implementation(),
                                           native_handle(),
                                           POLLOUT,
                                           wrap_token(std::forward<CompletionToken>(token)));
  }
protected:
  template<typename CompletionToken,
           typename... Args>
  auto post(CompletionToken token,
            Args&&... args)
  {
    using signature_type = void(std::decay_t<Args>...);
    using async_result_type = boost::asio::async_result<std::decay_t<CompletionToken>,
                                                        signature_type>;
    using completion_handler_type = typename async_result_type::completion_handler_type;
    completion_handler_type h(std::forward<CompletionToken>(token));
    auto ex = boost::asio::get_associated_executor(h,
                                                   get_executor());
    auto alloc = boost::asio::get_associated_allocator(h,
                                                       std::allocator<void>());
    ex.post(std::bind(std::move(h),
                      std::forward<Args>(args)...),
            alloc);
  }
  template<bool In,
           typename Signature,
           typename Invoker,
           typename CompletionToken>
  auto async_poll_then(Invoker i,
                       CompletionToken&& token)
  {
    using async_result_type = boost::asio::async_result<std::decay_t<CompletionToken>,
                                                        Signature>;
    using completion_handler_type = typename async_result_type::completion_handler_type;
    completion_handler_type h(std::forward<CompletionToken>(token));
    async_result_type result(h);
    detail::poll_file_op op(std::move(i),
                            std::move(h));
    if constexpr (In) {
      async_poll_in(std::move(op));
    } else {
      async_poll_out(std::move(op));
    }
    return result.get();
  }
private:
  template<bool In,
           typename BufferSequence,
           typename Invoker,
           typename CompletionToken>
  auto async_impl(BufferSequence bs,
                  Invoker i,
                  CompletionToken&& token)
  {
    if (boost::asio::buffer_size(bs)) {
      return async_poll_then<In,
                             signature>(std::move(i),
                                        std::forward<CompletionToken>(token));
    }
    return post(std::forward<CompletionToken>(token),
                std::error_code(),
                std::size_t(0));
  }
public:
  template<typename MutableBufferSequence,
           typename CompletionToken>
  auto async_read_some(MutableBufferSequence mb,
                       CompletionToken&& token)
  {
    return async_impl<true>(mb,
                            [fd = native_handle(),
                             mb](auto ec,
                                 auto h)
                            {
                              std::size_t bytes_transferred = 0;
                              if (!ec) {
                                bytes_transferred = asio::read(fd,
                                                               mb,
                                                               ec);
                              }
                              h(ec,
                                bytes_transferred);
                            },
                            std::forward<CompletionToken>(token));
  }
  template<typename ConstBufferSequence,
           typename CompletionToken>
  auto async_write_some(ConstBufferSequence cb,
                        CompletionToken&& token)
  {
    return async_impl<false>(cb,
                            [fd = native_handle(),
                             cb](auto ec,
                                 auto h)
                            {
                              std::size_t bytes_transferred = 0;
                              if (!ec) {
                                bytes_transferred = asio::write(fd,
                                                                cb,
                                                                ec);
                              }
                              h(ec,
                                bytes_transferred);
                            },
                            std::forward<CompletionToken>(token));
  }
};

}

namespace boost::asio {

template<typename CompletionHandler,
         typename Invoker,
         typename Allocator>
class associated_allocator<::asio_uring::asio::detail::poll_file_op<CompletionHandler,
                                                                    Invoker>,
                           Allocator>
{
public:
  using type = typename associated_allocator<CompletionHandler,
                                             Allocator>::type;

  static auto get(const ::asio_uring::asio::detail::poll_file_op<CompletionHandler,
                                                                 Invoker>& h,
                  const Allocator& alloc = Allocator())
  {
    return asio::get_associated_allocator(h.completion_handler(),
                                          alloc);
  }
};

template<typename CompletionHandler,
         typename Invoker,
         typename Executor>
class associated_executor<::asio_uring::asio::detail::poll_file_op<CompletionHandler,
                                                                   Invoker>,
                          Executor>
{
public:
  using type = typename associated_executor<CompletionHandler,
                                            Executor>::type;

  static auto get(const ::asio_uring::asio::detail::poll_file_op<CompletionHandler,
                                                                 Invoker>& h,
                  const Executor& ex = Executor())
  {
    return asio::get_associated_executor(h.completion_handler(),
                                         ex);
  }
};

}
