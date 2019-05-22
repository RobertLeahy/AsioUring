add_executable(asio_beast_example beast_example.cpp)
target_link_libraries(asio_beast_example Boost::boost
                                         asio
                                         Threads::Threads)
