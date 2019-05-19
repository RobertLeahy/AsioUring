asio_uring_add_library(asio SOURCES basic_io_object.cpp
                                    completion_handler.cpp
                                    execution_context.cpp
                                    fd_completion_handler.cpp
                                    fd_completion_token.cpp
                                    iovec.cpp
                                    service.cpp
                            LIBRARIES Boost::boost
                                      Boost::system
                                      core
                                      Uring::Uring)
add_subdirectory(tests)
