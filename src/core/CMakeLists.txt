asio_uring_add_library(core SOURCES callable_storage.cpp
                                    eventfd.cpp
                                    eventfd_queue.cpp
                                    execution_context.cpp
                                    fd.cpp
                                    spin_lock.cpp
                                    uring.cpp
                            LIBRARIES Uring::Uring)
add_subdirectory(tests)
