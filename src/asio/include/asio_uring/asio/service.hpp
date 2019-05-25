/**
 *  \file
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <asio_uring/asio/completion_handler.hpp>
#include <asio_uring/asio/iovec.hpp>
#include <asio_uring/liburing.hpp>
#include <asio_uring/service.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/execution_context.hpp>
#include <boost/system/error_code.hpp>
#include "execution_context.hpp"
#include <sys/uio.h>

namespace asio_uring::asio {

/**
 *  Derives from \ref asio_uring::service and
 *  `boost::asio::execution_context::service` thereby
 *  modeling `IoObjectService` (which
 *  \ref asio_uring::service already does) and
 *  `Service`.
 *
 *  Extends the functionality offered by
 *  \ref asio_uring::service by integrating with
 *  the guarantees of Boost.Asio (which
 *  \ref asio_uring::service deliberately does not
 *  do to remain agnostic between Boost.Asio and
 *  possible implementations based on the
 *  Networking TS).
 */
class service : public asio_uring::service,
                public boost::asio::execution_context::service
{
private:
  using rw_signature = void(boost::system::error_code,
                            std::size_t);
  using poll_signature = void(boost::system::error_code);
  using fsync_signature = poll_signature;
  using rw_result_type = std::pair<boost::system::error_code,
                                   std::size_t>;
  static rw_result_type to_rw_result(int) noexcept;
  static boost::system::error_code to_poll_add_result(int) noexcept;
  static boost::system::error_code to_poll_remove_result(int) noexcept;
  static boost::system::error_code to_fsync_result(int) noexcept;
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
  /**
   *  A type alias for this type.
   *
   *  Required by the `Service` concept.
   */
  using key_type = service;
  /**
   *  Creates a service which is associated with a
   *  particular `boost::asio::execution_context`.
   *
   *  \warning
   *    The `Service` concept requires that this constructor
   *    be callable with a sole argument of type
   *    `boost::asio::execution_context&` however this service
   *    requires access to a \ref asio_uring::execution_context
   *    to dispatch asynchronous operations in the `io_uring`
   *    managed thereby. Accordingly it will blindly downcast
   *    the given reference to this type and it is up to the caller
   *    to ensure the dynamic type of the reference is
   *    appropriate to enable this without undefined behavior.
   *
   *  \param [in] ctx
   *    A reference to a `boost::asio::execution_context` to
   *    associate with this object. This reference must remain
   *    valid for the lifetime of this object or the behavior is
   *    undefined with one exception: If this object's lifetime
   *    is managed by the set of services offered by the referred
   *    `boost::asio::execution_context` then the reference may
   *    be invalidated by entering that object's destructor without
   *    undefined behavior (in this case `shutdown` will be
   *    invoked on this object as is usual for the Boost.Asio
   *    service teardown process).
   */
  explicit service(boost::asio::execution_context& ctx);
  /**
   *  Obtains a reference to the associated \ref execution_context.
   *
   *  Note that this reference is the result of downcasting
   *  the sole argument to this object's constructor.
   *
   *  \return
   *    A reference to the associated \ref execution_context.
   */
  execution_context& context() const noexcept;
#ifndef ASIO_URING_DOXYGEN_RUNNING
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
#endif
private:
  virtual void shutdown() noexcept override;
};

}
