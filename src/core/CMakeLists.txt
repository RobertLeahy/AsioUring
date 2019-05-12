asio_uring_add_library(core SOURCES eventfd.cpp
                                    eventfd_queue.cpp
                                    fd.cpp
                                    lockfree_node.cpp
                                    lockfree_node_pool.cpp
                                    lockfree_queue.cpp
                                    spin_lock.cpp
                                    uring.cpp
                            LIBRARIES Uring::Uring)
add_subdirectory(tests)
