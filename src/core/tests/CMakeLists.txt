asio_uring_add_test(core
                    SOURCES callable_storage.cpp
                            connect.cpp
                            eventfd.cpp
                            eventfd_queue.cpp
                            execution_context.cpp
                            fd.cpp
                            main.cpp
                            read.cpp
                            service.cpp
                            spin_lock.cpp
                            uring.cpp
                            write.cpp
                    LIBRARIES Boost::boost
                              Catch2::Catch2
                              testing
                              Threads::Threads)
