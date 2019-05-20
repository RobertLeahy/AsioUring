asio_uring_add_test(asio
                    SOURCES accept_file.cpp
                            async_file.cpp
                            basic_io_object.cpp
                            completion_handler.cpp
                            connect_file.cpp
                            execution_context.cpp
                            fd_completion_handler.cpp
                            fd_completion_token.cpp
                            file_object.cpp
                            iovec.cpp
                            main.cpp
                            poll_file.cpp
                            read.cpp
                            service.cpp
                            write.cpp
                    LIBRARIES Boost::boost
                              Boost::system
                              Catch2::Catch2
                              testing
                              Threads::Threads)
