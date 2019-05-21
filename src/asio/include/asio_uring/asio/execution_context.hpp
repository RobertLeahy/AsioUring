/**
 *  \file
 */

#pragma once

#include <asio_uring/execution_context.hpp>
#include <boost/asio/execution_context.hpp>

namespace asio_uring::asio {

/**
 *  An `ExecutionContext` which is derived from both
 *  \ref asio_uring::execution_context and
 *  `boost::asio::execution_context` so that:
 *
 *  - I/O may be performed via `io_uring`
 *  - Services may be obtained as expected in the
 *    model of Boost.Asio
 */
class execution_context : public asio_uring::execution_context,
                          public boost::asio::execution_context
{
public:
  using asio_uring::execution_context::execution_context;
  ~execution_context() noexcept;
};

}
