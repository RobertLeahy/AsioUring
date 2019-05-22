asio_uring_add_library(asio SOURCES accept_file.cpp
                                    async_file.cpp
                                    basic_io_object.cpp
                                    completion_handler.cpp
                                    connect_file.cpp
                                    error_code.cpp
                                    execution_context.cpp
                                    fd_completion_handler.cpp
                                    fd_completion_token.cpp
                                    file_object.cpp
                                    iovec.cpp
                                    poll_file.cpp
                                    read.cpp
                                    service.cpp
                                    write.cpp
                            LIBRARIES Boost::boost
                                      Boost::system
                                      core
                                      Uring::Uring)
add_subdirectory(example)
add_subdirectory(tests)
