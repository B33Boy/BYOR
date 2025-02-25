#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

#include <netinet/ip.h>

class ISocketWrapper
{
public:
    virtual ~ISocketWrapper() = default;
    virtual int socket(int domain, int type, int protocol) const = 0;
    virtual int bind(int const fd, sockaddr const* addr, socklen_t addrlen) const = 0;
    virtual int listen(int const fd, int const backlog) const = 0;
    virtual int accept(int const fd, sockaddr* addr, socklen_t* addrlen) const = 0;
    virtual int close(int const fd) const = 0;

    virtual int setsockopt(int const fd) const = 0;
    virtual int fcntl(int const fd, int op) const = 0;
    virtual int fcntl(int const fd, int op, int arg) const = 0;
};

class SocketWrapper : public ISocketWrapper
{
public:
    int socket(int const domain, int const type, int const protocol) const override;
    int bind(int const fd, sockaddr const* addr, socklen_t addrlen) const override;
    int listen(int const fd, int backlog) const override;
    int accept(int const fd, sockaddr* addr, socklen_t* addrlen) const override;
    int close(int const fd) const override;

    int setsockopt(int const fd) const override;
    int fcntl(int const fd, int op) const override;
    int fcntl(int const fd, int op, int arg) const override;
};

#endif