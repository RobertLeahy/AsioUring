asio_uring_add_test(asio
                    SOURCES completion_handler.cpp
                            execution_context.cpp
                            main.cpp
                    LIBRARIES Boost::boost
                              Catch2::Catch2
                              Threads::Threads)
