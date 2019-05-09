asio_uring_add_test(core
                    SOURCES eventfd.cpp
                            fd.cpp
                            main.cpp
                            uring.cpp
                    LIBRARIES Catch2::Catch2)
