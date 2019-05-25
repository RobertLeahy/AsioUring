# Asio Uring

## Introduction

Asio Uring adapts the [`io_uring`](http://kernel.dk/io_uring.pdf) functionality added in the Linux kernel 5.1 to [the model of Boost.Asio](https://www.youtube.com/watch?v=hdRpCo94_C4) with the framework laid for future adaptation to the model of the Networking TS. This enables native asynchronous file I/O within the context of Boost.Asio.

## Important Notes

### Proactor- vs. Reactor-Style I/O

`io_uring` provides two broad ways to interact with files asynchronously:

1. Proactor-style I/O using asynchronous analogs to [`preadv2` and `pwritev2`](https://manpages.debian.org/testing/manpages-dev/preadv2.2.en.html)
2. Reactor-style I/O using an asynchronous analog to [`poll`](http://man7.org/linux/man-pages/man2/poll.2.html)

As a user of this library you don't need to concern yourself with the details of this (that's the point of the library) other than to be aware that certain classes of file descriptors work with one but not the other.

File descriptors which represent actual files in the filesystem only work with the former (and should be referred to `asio_uring::asio::async_file`) whereas file descriptors which don't represent actual files (e.g. sockets) only work with the latter (and should be referred to `asio_uring::asio::poll_file`).

You'll know you've done it wrong if your application hangs and the stack trace indicates it's stuck submitting work to the `io_uring`.

### Multithreading

It is not safe to simultaneously surrender multiple threads to `asio_uring::asio::execution_context` to use to run work. Put formally:

If two threads A and B include calls to any of the following member functions of `asio_uring::asio::execution_context`:

- `restart`
- `run`
- `run_one`
- `poll`
- `poll_one`

And it is not the case that the call in A inter-thread happens before the call in B, or vice versa, then the behavior is undefined.

Note that the [`Executor` concept](https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/Executor1.html) requires certain functions be thread safe (i.e. that they "not introduce data races as a result of concurrent calls to those functions from different threads") and `asio_uring::asio::execution_context::executor_type` abides by this. Only the execution context itself is not thread safe.

## Usage

As a user of the library you will interact directly with the following classes:

- `asio_uring::asio::execution_context`: A model of the [`ExecutionContext` concept](https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/ExecutionContext.html) implemented on top of an `io_uring`
- `asio_uring::fd`: An RAII wrapper over a Linux file descriptor which has a unary constructor which checks its argument against `-1` and throws a `std::system_error` encapsulating `errno` if the argument is `-1` (simplifies interoperating with system calls that generate file descriptors)
- `asio_uring::asio::async_file`: An I/O object which encapsulates a file descriptor for which proactor-style I/O is appropriate (models the Boost.Asio concepts [`AsyncRandomAccessReadDevice`](https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/AsyncRandomAccessReadDevice.html) and [`AsyncRandomAccessWriteDevice`](https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/AsyncRandomAccessWriteDevice.html))
- `asio_uring::asio::poll_file`: An I/O object which encapsulates a file descripctor for which reactor-style I/O is appropriate (models the Boost.Asio concepts [`AsyncReadStream`](https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/AsyncReadStream.html) and [`AsyncWriteStream`](https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/AsyncWriteStream.html))
- `asio_uring::asio::connect_file`: Adds `connect` support to `asio_uring::asio::poll_file`
- `asio_uring::asio::accept_file`: Wraps a file descriptor for the sole purpose of performing [`accept4`](https://linux.die.net/man/2/accept4) calls

Note that unlike Boost.Asio you will interact directly with file descriptors (via the owning wrapper `asio_uring::fd`) and that for reactor-style I/O you are expected to provide file descriptors which are already in non-blocking mode (the library cannot be expected to do this for you).

The expectation that your file descriptors will already be in non-blocking mode is motivated by the fact that:

- [`fcntl`](http://man7.org/linux/man-pages/man2/fcntl.2.html) is a system call
- Minimizing system calls is desirable (for performance)
- Many system calls can set the non-blocking flag as part of another system call (for example calls to [`socket`](http://man7.org/linux/man-pages/man2/socket.2.html) and [`accept4`](https://linux.die.net/man/2/accept4) with `SOCK_NONBLOCK`)

Accordingly the library gives the user the opportunity to minimize system calls (by fusing setting the non-blocking flag with another system call) and as a consequence expects file descriptors to be in non-blocking mode when they are presented to it.

Note that `asio_uring::asio::accept_file` uses [`accept4`](https://linux.die.net/man/2/accept4) with `SOCK_NONBLOCK` and therefore provides a non-blocking file descriptor while only making a single system call.

## Examples

### File I/O

```c++
#include <iostream>
#include <string_view>
#include <utility>
#include <asio_uring/asio/execution_context.hpp>
#include <asio_uring/asio/async_file.hpp>
#include <asio_uring/fd.hpp>
#include <boost/asio/buffer.hpp>
#include <fcntl.h>

// ...

// Open file using system call, the
// fd wrapper class will automatically
// throw from its constructor if this
// fails
asio_uring::fd file(::open("/path/to/file",
                           O_RDONLY));
// Creates the io_uring and the context
// for performing asynchronous I/O, the
// argument to the constructor is the size
// of the ring
asio_uring::asio::execution_context ctx(100);
// Creates an I/O object which can be used
// to read from and write to the file
// (although since we opened with O_RDONLY
// we can't actually read), note how we
// surrender ownership of the file descriptor
// to the I/O object
asio_uring::asio::async_file io(ctx,
                                std::move(file));
// We setup a read of up to 1024 bytes, note
// that the read itself may proceed in the
// background but the completion handler (i.e.
// the lambda) won't be actually invoked until
// we surrender a thread to the execution_context
// to use to run some work
char buffer[1024];
auto read_handler = [&](auto ec,
                        auto bytes_transferred)
{
  if (ec) {
    std::cerr << "ERROR: " << ec.message() << std::endl;
    return;
  }
  std::string_view sv(buffer,
                      bytes_transferred);
  std::cout << "Read " << bytes_transferred << " bytes:\n"
            << sv << std::endl;
};
io.async_read_some_at(0,
                      boost::asio::buffer(buffer),
                      read_handler);
// Here we actually surrender the current thread
// to the execution_context to use to run some
// work, this particular call will block until
// there's no work left
ctx.run();
```

### Socket I/O

There is an included fully worked example program which demonstrates interoperating with [Boost.Beast](https://www.boost.org/doc/libs/1_70_0/libs/beast/doc/html/index.html): [`src/asio/example/beast_example.cpp`](src/asio/example/beast_example.cpp).

## Dependencies

- Boost (tested against 1.70.0)
- C++17 compiler (tested against GCC 8.3.0)
- CMake >=3.8 (tested against 3.13.4)
- [liburing](http://git.kernel.dk/cgit/liburing/)
- Linux kernel >=5.1 (tested against 5.1 on Ubuntu 19.04)

The following are optional:

- [Catch2](https://github.com/catchorg/Catch2) (if provided the test suite will be built)
- [Doxygen](http://www.doxygen.nl/) (if provided you can build documentation)

## Building

```bash
cmake .. -DBOOST_ROOT=/path/to/boost -DURING_INCLUDE_DIR=/path/to/liburing/src -DURING_LIBRARY=/path/to/liburing.a
cmake --build .
```

If [Catch2](https://github.com/catchorg/Catch2) was provided you can then run the tests via:

```bash
ctest
```

If [Doxygen](http://www.doxygen.nl/) was provided you can build the documentation via:

```bash
cmake --build . --target doc
```

## Installation

`cmake --build . --target install`
