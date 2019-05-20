asio_uring_add_library(core SOURCES accept.cpp
                                    callable_storage.cpp
                                    connect.cpp
                                    eventfd.cpp
                                    eventfd_queue.cpp
                                    execution_context.cpp
                                    fd.cpp
                                    read.cpp
                                    service.cpp
                                    spin_lock.cpp
                                    uring.cpp
                                    write.cpp
                            LIBRARIES Boost::boost
                                      Uring::Uring)
add_subdirectory(tests)
