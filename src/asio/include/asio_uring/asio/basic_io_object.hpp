/**
 *  \file
 */

#pragma once

#include <cassert>
#include <utility>
#include <boost/asio/execution_context.hpp>

namespace asio_uring::asio {

/**
 *  A partial reimplementation (conditional movability is
 *  not supported) of `boost::asio::basic_io_object` to
 *  support I/O objects whose associated execution context
 *  is not `boost::asio::io_context`.
 *
 *  \tparam ExecutionContext
 *    The associated execution context.
 *  \tparam IoObjectService
 *    The I/O Object Service which models the `IoObjectService`
 *    concept from Boost.Asio.
 */
template<typename ExecutionContext,
         typename IoObjectService>
class basic_io_object {
public:
  /**
   *  The executor type of the associated execution context.
   */
  using executor_type = typename ExecutionContext::executor_type;
  /**
   *  A type alias for the second template parameter.
   */
  using service_type = IoObjectService;
  /**
   *  The implementation handle type for the associated
   *  \ref service_type "service".
   */
  using implementation_type = typename service_type::implementation_type;
  basic_io_object() = delete;
  basic_io_object(const basic_io_object&) = delete;
  basic_io_object& operator=(const basic_io_object&) = delete;
  /**
   *  Obtains a reference to the \ref service_type "associated service type"
   *  from the set of services managed by the associated execution
   *  context and then initializes the stored
   *  \ref implementation_type "handle" using that service.
   *
   *  \param [in] ctx
   *    A reference to the associated execution context. This
   *    reference must remain valid for the lifetime of this object
   *    or the behavior is undefined.
   */
  explicit basic_io_object(ExecutionContext& ctx)
    : svc_(boost::asio::use_service<service_type>(ctx))
  {
    svc_.construct(impl_);
  }
  /**
   *  Uses the \ref service_type "service" associated with another
   *  object to transfer the \ref implementation_type "implementation"
   *  from that object to this newly-created object.
   *
   *  \param [in] other
   *    The object from which to transfer ownership.
   */
  basic_io_object(basic_io_object&& other) noexcept(noexcept(std::declval<service_type&>().move_construct(std::declval<implementation_type&>(),
                                                                                                          std::declval<implementation_type&>())))
    : svc_(other.svc_)
  {
    svc_.move_construct(impl_,
                        other.impl_);
  }
  /**
   *  Uses the \ref service_type "service" associated with this
   *  object to transfer the \ref implementation_type "implementation"
   *  from that object to this object replacing any
   *  \ref implementation_type "implementation" this object owned
   *  in the process.
   *
   *  \param [in] rhs
   *    The object from which to transfer ownership.
   *
   *  \return
   *    A reference to this object.
   */
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
  /**
   *  Uses the associated \ref service_type "service" to destroy
   *  the owned \ref implementation_type "implementation".
   */
  ~basic_io_object() noexcept {
    svc_.destroy(impl_);
  }
  /**
   *  Retrieves an \ref executor_type "executor" associated with
   *  the associated execution context.
   *
   *  \return
   *    An \ref executor_type "executor".
   */
  executor_type get_executor() const noexcept {
    return svc_.context().get_executor();
  }
  /**
   *  @{
   *  Retrieves a reference to the owned
   *  \ref implementation_type "implementation".
   *
   *  \return
   *    A reference to the \ref implementation_type "implementation".
   */
  implementation_type& get_implementation() noexcept {
    return impl_;
  }
  const implementation_type& get_implementation() const noexcept {
    return impl_;
  }
  /**
   *  @}
   *  Retrieves a reference to the associated
   *  \ref service_type "service".
   *
   *  Note that a reference to the same object may be
   *  obtained via `boost::asio::use_service<service_type>(ctx)`
   *  where `ctx` is the execution context passed to the
   *  constructor used to create this object however
   *  using this function bypasses the lookup in the
   *  set of services associated with that execution
   *  context.
   *
   *  \return
   *    A reference to the associated
   *    \ref service_type "service".
   */
  service_type& get_service() noexcept {
    return svc_;
  }
private:
  service_type&       svc_;
  implementation_type impl_;
};

}
