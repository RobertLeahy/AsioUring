/**
 *  \file
 */

#pragma once

#include <asio_uring/service.hpp>
#include <boost/asio/execution_context.hpp>
#include "execution_context.hpp"

namespace asio_uring::asio {

class service : public asio_uring::service,
                public boost::asio::execution_context::service
{
public:
  using key_type = service;
  explicit service(boost::asio::execution_context& ctx);
  execution_context& context() const noexcept;
private:
  virtual void shutdown() noexcept override;
};

}
