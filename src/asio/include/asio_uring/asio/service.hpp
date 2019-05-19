/**
 *  \file
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <system_error>
#include <type_traits>
#include <utility>
#include <asio_uring/asio/completion_handler.hpp>
#include <asio_uring/asio/iovec.hpp>
#include <asio_uring/liburing.hpp>
#include <asio_uring/service.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/execution_context.hpp>
#include "execution_context.hpp"
#include <sys/uio.h>

namespace asio_uring::asio {

class service : public asio_uring::service,
                public boost::asio::execution_context::service
{
private:
  using rw_signature = void(std::error_code,
                            std::size_t);
  using poll_signature = void(std::error_code);
  using fsync_signature = poll_signature;
  using rw_result_type = std::pair<std::error_code,
                                   std::size_t>;
  static rw_result_type to_rw_result(int) noexcept;
  static std::error_code to_poll_add_result(int) noexcept;
  static std::error_code to_poll_remove_result(int) noexcept;
  static std::error_code to_fsync_result(int) noexcept;
  template<typename Function>
  static auto make_rw_completion(Function f) {
    return [func = std::move(f)](auto&& cqe) mutable {
      auto [ec, bytes_transferred] = to_rw_result(cqe.res);
      func(ec,
           bytes_transferred);
    };
  }
  template<typename BufferSequence,
           typename CompletionToken>
  auto initiate_rw_some_at(implementation_type& impl,
                           int fd,
                           void (*prep)(::io_uring_sqe*,
                                        int,
                                        ::iovec*,
                                        unsigned,
                                        ::off_t),
                           std::uint64_t o,
                           BufferSequence bs,
                           CompletionToken&& token)
  {
    using async_result_type = boost::asio::async_result<std::decay_t<CompletionToken>,
                                                        rw_signature>;
    using completion_handler_type = typename async_result_type::completion_handler_type;
    completion_handler_type h(std::forward<CompletionToken>(token));
    async_result_type result(h);
    completion_handler wrapper(std::move(h),
                               context().get_executor());
    std::size_t n(std::distance(boost::asio::buffer_sequence_begin(bs),
                                boost::asio::buffer_sequence_end(bs)));
    auto alloc = wrapper.get_allocator();
    initiate(impl,
             n,
             [&](auto&& sqe,
                 auto iovs,
                 auto) noexcept
             {
               to_iovecs(bs,
                         iovs);
               prep(&sqe,
                    fd,
                    iovs,
                    n,
                    o);
             },
             make_rw_completion(std::move(wrapper)),
             alloc);
    return result.get();
  }
public:
  using key_type = service;
  explicit service(boost::asio::execution_context& ctx);
  execution_context& context() const noexcept;
  template<typename MutableBufferSequence,
           typename CompletionToken>
  auto initiate_read_some_at(implementation_type& impl,
                             int fd,
                             std::uint64_t o,
                             MutableBufferSequence mb,
                             CompletionToken&& token)
  {
    return initiate_rw_some_at(impl,
                               fd,
                               &::io_uring_prep_readv,
                               o,
                               mb,
                               std::forward<CompletionToken>(token));
  }
  template<typename ConstBufferSequence,
           typename CompletionToken>
  auto initiate_write_some_at(implementation_type& impl,
                              int fd,
                              std::uint64_t o,
                              ConstBufferSequence cb,
                              CompletionToken&& token)
  {
    return initiate_rw_some_at(impl,
                               fd,
                               &::io_uring_prep_writev,
                               o,
                               cb,
                               std::forward<CompletionToken>(token));
  }
  template<typename CompletionToken>
  auto initiate_poll_add(implementation_type& impl,
                         int fd,
                         short mask,
                         CompletionToken&& token)
  {
    using async_result_type = boost::asio::async_result<std::decay_t<CompletionToken>,
                                                        poll_signature>;
    using completion_handler_type = typename async_result_type::completion_handler_type;
    completion_handler_type h(std::forward<CompletionToken>(token));
    async_result_type result(h);
    completion_handler wrapper(std::move(h),
                               context().get_executor());
    auto alloc = wrapper.get_allocator();
    initiate(impl,
             [&](auto&& sqe,
                 auto) noexcept
             {
               ::io_uring_prep_poll_add(&sqe,
                                        fd,
                                        mask);
             },
             [w = std::move(wrapper)](auto&& cqe) mutable {
               w(to_poll_add_result(cqe.res));
             },
             alloc);
    return result.get();
  }
  template<typename CompletionToken>
  auto initiate_poll_remove(implementation_type& impl,
                            const void* user_data,
                            CompletionToken&& token)
  {
    using async_result_type = boost::asio::async_result<std::decay_t<CompletionToken>,
                                                        poll_signature>;
    using completion_handler_type = typename async_result_type::completion_handler_type;
    completion_handler_type h(std::forward<CompletionToken>(token));
    async_result_type result(h);
    completion_handler wrapper(std::move(h),
                               context().get_executor());
    auto alloc = wrapper.get_allocator();
    initiate(impl,
             [&](auto&& sqe,
                 auto) noexcept
             {
               ::io_uring_prep_poll_remove(&sqe,
                                           const_cast<void*>(user_data));
             },
             [w = std::move(wrapper)](auto&& cqe) mutable {
               w(to_poll_remove_result(cqe.res));
             },
             alloc);
    return result.get();
  }
  template<typename CompletionToken>
  auto initiate_fsync(implementation_type& impl,
                      int fd,
                      bool fdatasync,
                      CompletionToken&& token)
  {
    using async_result_type = boost::asio::async_result<std::decay_t<CompletionToken>,
                                                        poll_signature>;
    using completion_handler_type = typename async_result_type::completion_handler_type;
    completion_handler_type h(std::forward<CompletionToken>(token));
    async_result_type result(h);
    completion_handler wrapper(std::move(h),
                               context().get_executor());
    auto alloc = wrapper.get_allocator();
    initiate(impl,
             [&](auto&& sqe,
                 auto) noexcept
             {
               ::io_uring_prep_fsync(&sqe,
                                     fd,
                                     fdatasync ? IORING_FSYNC_DATASYNC : 0);
             },
             [w = std::move(wrapper)](auto&& cqe) mutable {
               w(to_fsync_result(cqe.res));
             },
             alloc);
    return result.get();
  }
private:
  virtual void shutdown() noexcept override;
};

}
