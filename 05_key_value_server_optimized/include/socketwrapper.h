#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

#include <fcntl.h>      // F_GETFL, F_SETFL, O_NONBLOCK
#include <netinet/ip.h> // sockaddr_in
#include <sys/socket.h> // socket(), setsockopt(), bind(), listen(), accept()
#include <unistd.h>     // close(), read(), write()

template <typename Derived>
class ISocketWrapperBase
{
public:
    int socket(int const domain, int const type, int const protocol) const
    {
        return derived().socket_impl(domain, type, protocol);
    }

    int bind(int const fd, sockaddr const* addr, socklen_t addrlen) const
    {
        return derived().bind_impl(fd, addr, addrlen);
    }

    int listen(int const fd, int backlog) const
    {
        return derived().listen_impl(fd, backlog);
    }

    int accept(int const fd, sockaddr* addr, socklen_t* addrlen) const
    {
        return derived().accept_impl(fd, addr, addrlen);
    }

    int close(int const fd) const
    {
        return derived().close_impl(fd);
    }

    int setsockopt(int const fd) const
    {
        return derived().setsockopt_impl(fd);
    }

    int fcntl(int const fd, int op) const
    {
        return derived().fcntl_impl(fd, op);
    }

    int fcntl(int const fd, int op, int arg) const
    {
        return derived().fcntl_impl(fd, op, arg);
    }

private:
    // Enforce compile-time check to prevent misuse
    ISocketWrapperBase() {};
    friend Derived;

    // Simplify syntax for delegating work to derived class
    Derived const& derived() const noexcept
    {
        return static_cast<Derived const&>(*this);
    }
};

class SocketWrapper final : public ISocketWrapperBase<SocketWrapper>
{
public:
    int socket_impl(int const domain, int const type, int const protocol) const
    {
        return ::socket(domain, type, protocol);
    }

    int bind_impl(int const fd, sockaddr const* addr, socklen_t addrlen) const
    {
        return ::bind(fd, addr, addrlen);
    }

    int listen_impl(int const fd, int backlog) const
    {
        return ::listen(fd, backlog);
    }

    int accept_impl(int const fd, sockaddr* addr, socklen_t* addrlen) const
    {
        return ::accept(fd, addr, addrlen);
    }

    int close_impl(int const fd) const
    {
        return ::close(fd);
    }

    int setsockopt_impl(int const fd) const
    {
        int opt = 1;
        return ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }

    int fcntl_impl(int const fd, int op) const
    {
        return ::fcntl(fd, op);
    }

    int fcntl_impl(int const fd, int op, int arg) const
    {
        return ::fcntl(fd, op, arg);
    }
};

#endif