#include "server.h"

int main()
{
    // spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_LEVEL));

    constexpr uint16_t port{ 1234 };
    constexpr uint8_t max_clients{ 100 };

    SocketWrapper socket_wrapper;
    EpollWrapper epoll_wrapper(max_clients);

    Server<SocketWrapper, EpollWrapper> server(port, socket_wrapper, epoll_wrapper, max_clients);
    server.start();

    return 0;
}