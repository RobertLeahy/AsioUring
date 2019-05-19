asio_uring_add_test(asio
                    SOURCES completion_handler.cpp
                            execution_context.cpp
                            fd_completion_handler.cpp
                            iovec.cpp
                            main.cpp
                            service.cpp
                    LIBRARIES Boost::boost
                              Boost::system
                              Catch2::Catch2
                              testing
                              Threads::Threads)
