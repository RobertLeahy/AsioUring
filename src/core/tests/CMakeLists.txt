asio_uring_add_test(core
                    SOURCES eventfd.cpp
                            eventfd_queue.cpp
                            fd.cpp
                            main.cpp
                            lockfree_node.cpp
                            lockfree_node_pool.cpp
                            lockfree_queue.cpp
                            spin_lock.cpp
                            uring.cpp
                    LIBRARIES Boost::boost
                              Catch2::Catch2
                              testing)
