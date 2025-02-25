#include "socketwrapper.h"

#include <fcntl.h>      // F_GETFL, F_SETFL, O_NONBLOCK
#include <netinet/ip.h> // sockaddr_in
#include <sys/socket.h> // socket(), setsockopt(), bind(), listen(), accept()
#include <unistd.h>     // close(), read(), write()

int SocketWrapper::socket(int const domain, int const type, int const protocol) const
{
    return ::socket(domain, type, protocol);
}

int SocketWrapper::bind(int const fd, sockaddr const* addr, socklen_t addrlen) const
{
    return ::bind(fd, addr, addrlen);
}

int SocketWrapper::listen(int const fd, int const backlog) const
{
    return ::listen(fd, backlog);
}

int SocketWrapper::accept(int const fd, sockaddr* addr, socklen_t* addrlen) const
{
    return ::accept(fd, addr, addrlen);
}

int SocketWrapper::close(int const fd) const
{
    return ::close(fd);
}

int SocketWrapper::setsockopt(int const fd) const
{
    int opt = 1;
    return ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

int SocketWrapper::fcntl(int const fd, int op) const
{
    return ::fcntl(fd, op);
}

int SocketWrapper::fcntl(int const fd, int op, int arg) const
{
    return ::fcntl(fd, op, arg);
}
