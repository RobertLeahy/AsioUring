/**
 *  \file
 */

#pragma once

#include <cassert>
#include <utility>
#include <boost/asio/execution_context.hpp>

namespace asio_uring::asio {

template<typename ExecutionContext,
         typename IoObjectService>
class basic_io_object {
public:
  using executor_type = typename ExecutionContext::executor_type;
  using service_type = IoObjectService;
  using implementation_type = typename service_type::implementation_type;
  basic_io_object() = delete;
  basic_io_object(const basic_io_object&) = delete;
  basic_io_object& operator=(const basic_io_object&) = delete;
  explicit basic_io_object(ExecutionContext& ctx)
    : svc_(boost::asio::use_service<service_type>(ctx))
  {
    svc_.construct(impl_);
  }
  basic_io_object(basic_io_object&& obj) noexcept(noexcept(std::declval<service_type&>().move_construct(std::declval<implementation_type&>(),
                                                                                                        std::declval<implementation_type&>())))
    : svc_(obj.svc_)
  {
    svc_.move_construct(impl_,
                        obj.impl_);
  }
  basic_io_object& operator=(basic_io_object&& rhs) noexcept(noexcept(std::declval<service_type&>().move_assign(std::declval<implementation_type&>(),
                                                                                                                std::declval<service_type&>(),
                                                                                                                std::declval<implementation_type&>())))
  {
    assert(this != &rhs);
    svc_.move_assign(impl_,
                     rhs.svc_,
                     rhs.impl_);
    return *this;
  }
  ~basic_io_object() noexcept {
    svc_.destroy(impl_);
  }
  executor_type get_executor() const noexcept {
    return svc_.context().get_executor();
  }
  implementation_type& get_implementation() noexcept {
    return impl_;
  }
  const implementation_type& get_implementation() const noexcept {
    return impl_;
  }
  service_type& get_service() noexcept {
    return svc_;
  }
private:
  service_type&       svc_;
  implementation_type impl_;
};

}
