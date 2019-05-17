asio_uring_add_library(asio SOURCES completion_handler.cpp
                                    execution_context.cpp
                            LIBRARIES Boost::boost
                                      core
                                      Uring::Uring)
add_subdirectory(tests)
