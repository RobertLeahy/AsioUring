/**
 *  \file
 */

#pragma once

#include <asio_uring/execution_context.hpp>
#include <boost/asio/execution_context.hpp>

namespace asio_uring::asio {

class execution_context : public asio_uring::execution_context,
                          public boost::asio::execution_context
{
public:
  using asio_uring::execution_context::execution_context;
  ~execution_context() noexcept;
};

}
