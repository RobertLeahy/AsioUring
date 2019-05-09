asio_uring_add_library(core SOURCES uring.cpp
                            LIBRARIES Uring::Uring)
add_subdirectory(tests)
