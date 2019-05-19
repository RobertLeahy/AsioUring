/**
 *  \file
 */

#pragma once

#include <memory>
#include <type_traits>
#include <asio_uring/fd.hpp>
#include "basic_io_object.hpp"
#include "execution_context.hpp"
#include "fd_completion_handler.hpp"
#include "fd_completion_token.hpp"
#include "service.hpp"

namespace asio_uring::asio {

class file_object : public basic_io_object<execution_context,
                                           service>
{
private:
  using base = basic_io_object<execution_context,
                               service>;
public:
  using native_handle_type = fd::native_handle_type;
  using const_native_handle_type = fd::const_native_handle_type;
  file_object(execution_context& ctx,
              fd file);
  native_handle_type native_handle() noexcept;
  const_native_handle_type native_handle() const noexcept;
protected:
  template<typename CompletionHandler>
  auto wrap_handler(CompletionHandler&& h) noexcept(std::is_nothrow_move_constructible_v<std::decay_t<CompletionHandler>>) {
    return fd_completion_handler(std::forward<CompletionHandler>(h),
                                 fd_);
  }
  template<typename CompletionToken>
  auto wrap_token(CompletionToken&& token) noexcept(std::is_nothrow_move_constructible_v<std::decay_t<CompletionToken>>) {
    return fd_completion_token(std::forward<CompletionToken>(token),
                               fd_);
  }
  void reset() noexcept;
  long outstanding() const noexcept;
private:
  std::shared_ptr<fd> fd_;
};

}
