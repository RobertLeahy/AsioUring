#include <asio_uring/test/allocator.hpp>

namespace asio_uring::test::detail {

allocator_state::allocator_state() noexcept
  : allocate        (0),
    deallocate      (0),
    construct       (0),
    destroy         (0),
    allocate_throws (false),
    construct_throws(false)
{}

}
