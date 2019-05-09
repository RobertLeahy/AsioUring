asio_uring_add_library(core SOURCES fd.cpp
                                    uring.cpp
                            LIBRARIES Uring::Uring)
add_subdirectory(tests)
