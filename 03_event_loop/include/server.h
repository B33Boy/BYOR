#ifndef SERVER_H
#define SERVER_H

#include <cerrno>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include <arpa/inet.h>  // ntohs(), ntohl()
#include <fcntl.h>      // F_GETFL, F_SETFL, O_NONBLOCK
#include <netinet/ip.h> // sockaddr_in
#include <sys/socket.h> // socket(), setsockopt(), bind(), listen(), accept()
#include <unistd.h>     // close(), read(), write()

#include "epollwrapper.h"

class Server
{
private:
    static constexpr uint8_t DEFAULT_MAX_CLIENTS{ 10 };

public:
    Server(uint16_t  port, uint8_t max_clients = DEFAULT_MAX_CLIENTS) : server_fd_(-1), port_(port), max_clients_(max_clients), epoll_(max_clients_) {}

    ~Server() = default;
    Server(Server const& other) = delete;
    Server(Server&& other) = delete;
    Server& operator=(Server const& other) = delete;
    Server& operator=(Server&& other) = delete;

    void start();

private:
    int server_fd_;
    uint16_t const port_;
    uint8_t const max_clients_;

    EpollWrapper epoll_;

    void setup();
    void create_server_socket();
    void set_socket_options();
    void bind_socket();
    bool set_nonblocking(int fd);

    void handle_new_connections(int server_fd);
    void handle_read_event(int client_fd);
    void handle_write_event(int client_fd);
    void handle_close_event(int client_fd);
};

#endif