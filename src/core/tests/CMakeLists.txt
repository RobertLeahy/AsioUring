asio_uring_add_test(core
                    SOURCES fd.cpp
                            main.cpp
                            uring.cpp
                    LIBRARIES Catch2::Catch2)
