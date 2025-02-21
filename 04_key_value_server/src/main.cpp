#include "server.h"

int main()
{
    constexpr uint16_t port{ 1234 };
    constexpr uint8_t max_clients{ 100 };

    SocketWrapper socket_wrapper;
    EpollWrapper epoll_wrapper(max_clients);

    Server<SocketWrapper, EpollWrapper> server(port, max_clients, socket_wrapper, epoll_wrapper);
    server.start();

    return 0;
}