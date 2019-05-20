/**
 *  \file
 */

#pragma once

#include <cassert>
#include <memory>
#include <optional>
#include <system_error>
#include <utility>
#include <asio_uring/accept.hpp>
#include <asio_uring/fd.hpp>
#include "poll_file.hpp"

namespace asio_uring::asio {

class accept_file {
public:
  accept_file() = delete;
  accept_file(const accept_file&) = delete;
  accept_file(accept_file&&) = default;
  accept_file& operator=(const accept_file&) = delete;
  accept_file& operator=(accept_file&&) = default;
  accept_file(execution_context& ctx,
              fd file);
private:
  using signature = void(std::error_code,
                         fd);
  class impl : public poll_file,
               public std::enable_shared_from_this<impl>
  {
  public:
    using poll_file::poll_file;
    template<typename Address,
             typename CompletionToken>
    auto async_accept(Address* addr,
                      CompletionToken&& token)
    {
      auto i = [addr,
                self = shared_from_this()](auto ec,
                                           auto h) mutable
      {
        if (ec) {
          h(ec,
            fd());
          return;
        }
        std::optional<fd> accepted;
        if (addr) {
          accepted = accept(self->native_handle(),
                            *addr,
                            ec);
        } else {
          accepted = accept(self->native_handle(),
                            ec);
        }
        if (ec) {
          h(ec,
            fd());
          return;
        }
        if (accepted) {
          h(ec,
            std::move(*accepted));
          return;
        }
        self->async_accept(addr,
                           std::move(h));
      };
      return async_poll_then<true,
                             accept_file::signature>(std::move(i),
                                                     std::forward<CompletionToken>(token));
    }
  };
public:
  using executor_type = impl::executor_type;
  using implementation_type = impl::implementation_type;
  using service_type = impl::service_type;
  executor_type get_executor() const noexcept;
  implementation_type& get_implementation() noexcept;
  const implementation_type& get_implementation() const noexcept;
  service_type& get_service() noexcept;
  template<typename CompletionToken>
  auto async_accept(CompletionToken&& token) {
    assert(impl_);
    char* ptr = nullptr;
    return impl_->async_accept(ptr,
                               std::forward<CompletionToken>(token));
  }
  template<typename Address,
           typename CompletionToken>
  auto async_accept(Address& addr,
                    CompletionToken&& token)
  {
    assert(impl_);
    return impl_->async_accept(std::addressof(addr),
                               std::forward<CompletionToken>(token));
  }
private:
  using impl_type = std::shared_ptr<impl>;
  impl_type impl_;
};

}
