asio_uring_add_library(asio SOURCES completion_handler.cpp
                                    execution_context.cpp
                                    iovec.cpp
                                    service.cpp
                            LIBRARIES Boost::boost
                                      Boost::system
                                      core
                                      Uring::Uring)
add_subdirectory(tests)
