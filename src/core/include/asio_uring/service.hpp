/**
 *  \file
 */

#pragma once

#include <optional>
#include <type_traits>
#include <utility>
#include <vector>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>
#include <boost/intrusive/options.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include "callable_storage.hpp"
#include "execution_context.hpp"
#include "liburing.hpp"
#include <sys/uio.h>

namespace asio_uring {

class service {
private:
  using hook_type = boost::intrusive::list_member_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>>;
  using iovs_type = std::vector<::iovec>;
  class completion final : public execution_context::completion {
  friend class service;
  private:
    using function_type = callable_storage<256,
                                           void(::io_uring_cqe)>;
  public:
    explicit completion(service&);
    virtual void complete(const ::io_uring_cqe&) override;
    template<typename T,
             typename Allocator>
    void emplace(T&& t,
                 const Allocator& alloc) noexcept(std::is_nothrow_constructible_v<function_type,
                                                                                  T&&,
                                                                                  const Allocator&>)
    {
      assert(!wrapped_);
      wrapped_.emplace(std::forward<T>(t),
                       alloc);
      svc_.context().get_executor().on_work_started();
    }
    void reset() noexcept;
  private:
    service&                     svc_;
    hook_type                    service_;
    hook_type                    implementation_;
    std::optional<function_type> wrapped_;
    iovs_type                    iovs_;
  };
  template<hook_type(completion::*MemberPtr)>
  using list_t = boost::intrusive::list<completion,
                                        boost::intrusive::member_hook<completion,
                                                                      hook_type,
                                                                      MemberPtr>,
                                        boost::intrusive::constant_time_size<false>>;
  using implementation_list_type = list_t<&completion::implementation_>;
  class release_guard {
  public:
    release_guard(service&,
                  completion&) noexcept;
    release_guard(const release_guard&) = delete;
    release_guard(release_guard&&) = delete;
    release_guard& operator=(const release_guard&) = delete;
    release_guard& operator=(release_guard&&) = delete;
    ~release_guard() noexcept;
    void release() noexcept;
  private:
    service*    self_;
    completion& c_;
  };
public:
  class implementation_type {
  friend class service;
  public:
    implementation_type() = default;
    implementation_type(const implementation_type&) = delete;
    implementation_type(implementation_type&&) = delete;
    implementation_type& operator=(const implementation_type&) = delete;
    implementation_type& operator=(implementation_type&&) = delete;
  private:
    using list_type = implementation_list_type;
    class to_const_void_pointer {
    public:
      using result_type = const void*;
      using argument_type = const completion&;
      result_type operator()(argument_type arg) const noexcept;
    };
    list_type list_;
  public:
    using const_iterator = boost::transform_iterator<to_const_void_pointer,
                                                     list_type::const_iterator>;
    using iterator = const_iterator;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
  };
  explicit service(execution_context& ctx);
  service(const service&) = delete;
  service(service&&) = delete;
  service& operator=(const service&) = delete;
  service& operator=(service&&) = delete;
  ~service() noexcept;
  void shutdown() noexcept;
  execution_context& context() const noexcept;
  void construct(implementation_type& impl);
  void destroy(implementation_type& impl) noexcept;
  void move_construct(implementation_type& impl,
                      implementation_type& src) noexcept;
  void move_assign(implementation_type& impl,
                   service& svc,
                   implementation_type& src) noexcept;
  template<typename Function,
           typename T,
           typename Allocator>
  void initiate(implementation_type& impl,
                Function f,
                T&& t,
                const Allocator& alloc)
  {
    auto&& c = acquire(impl);
    release_guard g(*this,
                    c);
    c.emplace(std::forward<T>(t),
              alloc);
    auto&& sqe = ctx_.get_sqe();
    void* user_data = &c;
    static_assert(noexcept(f(sqe,
                             user_data)));
    f(sqe,
      user_data);
    submit(sqe,
           c);
    g.release();
  }
  template<typename Function,
           typename T,
           typename Allocator>
  void initiate(implementation_type& impl,
                std::size_t iovs,
                Function f,
                T&& t,
                const Allocator& alloc)
  {
    auto&& c = acquire(impl);
    release_guard g(*this,
                    c);
    c.iovs_ = acquire(iovs);
    c.emplace(std::forward<T>(t),
              alloc);
    auto&& sqe = ctx_.get_sqe();
    void* user_data = &c;
    static_assert(noexcept(f(sqe,
                             c.iovs_.data(),
                             user_data)));
    f(sqe,
      c.iovs_.data(),
      user_data);
    submit(sqe,
           c);
    g.release();
  }
private:
  completion& maybe_allocate();
  completion& acquire(implementation_type&);
  iovs_type acquire(iovs_type::size_type);
  void release(completion&) noexcept;
  void release(iovs_type&) noexcept;
  void submit(::io_uring_sqe&,
              completion&);
  using list_type = list_t<&completion::service_>;
  using iovs_cache_type = std::vector<iovs_type>;
  void destroy_list(list_type&) noexcept;
  execution_context& ctx_;
  list_type          free_;
  list_type          in_use_;
  iovs_cache_type    iovs_cache_;
};

}
