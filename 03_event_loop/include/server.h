#ifndef SERVER_H
#define SERVER_H

#include <cstring>
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
    static constexpr uint8_t DEFAULT_MAX_CLIENTS{ 100 };
    static constexpr uint8_t LEN_FIELD_SIZE{ 4 };
    static constexpr size_t MAX_MSG_FIELD_SIZE{ 32 << 20 }; // 32 MB, max valid size of a message
    static constexpr size_t READ_BUFFER_SIZE{ 64 * 1024 }; // 64 KB, max size for single read operations

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

    void create_server_socket();
    void set_socket_options() const noexcept;
    void bind_socket() const;
    bool set_nonblocking(int const fd) const noexcept;
    void setup_server();

    void handle_new_connections() noexcept;
    void handle_read_event(int const client_fd);
    bool try_request(Connection& conn) noexcept;
    void handle_write_event(int const client_fd);
    void handle_close_event(int const client_fd);
};

#endif