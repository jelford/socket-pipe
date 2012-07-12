# EchoServer

Usage:

    EchoServer INBOUND_ADDRESS INBOUND_PORT TARGET_ADDRESS TARGET_PORT

EchoServer will just forward all inbound data in `INBOUND_PORT` to the target
address and port. Along the way, it gets logged. The idea is to be able to help
diagnose problems in existing network applications, while abstracting away
the job of logging all the data sent/received.

## Build 

As far as I know, you shouldn't need to deal with any dependencies (it really is
just a thin wrapper); everything should be as simple as:

    cmake <source_dir>
    make

If you don't have `cmake`, then the following should do the trick:

    g++ -I Sockets/include -o EchoServer --std=c++11 ./main.cpp Sockets/lib/Socket.cpp

Note the `--std=c++11` flag; if you have trouble compiling on a different
compiler, the problem might be that it doesn't support the latest C++ (e.g. `auto`
and range-based iterators, which are both used in the `Sockets` library)

## License 

Provided as-is. You are free to copy, redistribute, use, statically link, modify,
and so on, with or without attribution. If you do find this useful, I'd like to
hear about it.

## Further work 

 * This is such a thin wrapper that it's hard to find seams. That said, if would be
    good to write some tests.
 * There is not yet any IPV6 support. I haven't made a great effort to get it
    working, since my router doesn't assign IPV6 addresses anyway, but it would
    be nice to have.
 * Other nice-to-haves might be e.g. support for other socket types. Current
    support is limited to TCP over IPV4. I've already mentioned IPV6, but it
    would also be nice to support UDP, UNIX Sockets, and so on.
