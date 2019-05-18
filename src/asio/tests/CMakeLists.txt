asio_uring_add_test(asio
                    SOURCES completion_handler.cpp
                            execution_context.cpp
                            iovec.cpp
                            main.cpp
                            service.cpp
                    LIBRARIES Boost::boost
                              Boost::system
                              Catch2::Catch2
                              Threads::Threads)
