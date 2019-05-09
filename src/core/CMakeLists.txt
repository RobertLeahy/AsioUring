asio_uring_add_library(core SOURCES eventfd.cpp
                                    fd.cpp
                                    uring.cpp
                            LIBRARIES Uring::Uring)
add_subdirectory(tests)
