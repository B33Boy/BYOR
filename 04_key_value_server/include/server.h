#ifndef SERVER_H
#define SERVER_H

#include "epollwrapper.h"
#include "socketwrapper.h"

#include <arpa/inet.h> // ntohs(), ntohl()
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h> // F_GETFL, F_SETFL, O_NONBLOCK
#include <iostream>
#include <map>
#include <memory>
#include <netinet/ip.h> // sockaddr_in
#include <stdexcept>
#include <sys/socket.h> // socket(), setsockopt(), bind(), listen(), accept()
#include <unistd.h>     // close(), read(), write()

enum class ResponseStatus : uint8_t
{
    RES_OK = 0,
    RES_ERR, // Err
    RES_NX,  // Key not found
};

struct Response
{
    ResponseStatus status{};
    std::vector<uint8_t> data{};
};

class Server
{
private:
    static constexpr uint8_t DEFAULT_MAX_CLIENTS{ 100 };
    static constexpr uint8_t LEN_FIELD_SIZE{ 4 };
    static constexpr size_t MAX_MSG_FIELD_SIZE{ 32 << 20 };
    static constexpr size_t READ_BUFFER_SIZE{ 64 * 1024 };
    static constexpr size_t MAX_CMD_ARGS = 200 * 1000;

public:
    Server(uint16_t port, std::unique_ptr<ISocketWrapper> sockwrapper = std::make_unique<SocketWrapper>(),
           std::unique_ptr<IEpollWrapper> epoll = std::make_unique<EpollWrapper>(DEFAULT_MAX_CLIENTS),
           uint8_t max_clients = DEFAULT_MAX_CLIENTS)
        : server_fd_(-1), port_(port), sockwrapper_(std::move(sockwrapper)), epoll_(std::move(epoll)),
          max_clients_(max_clients)
    {
    }

    ~Server() = default;
    Server(Server const& other) = delete;
    Server(Server&& other) = delete;
    Server& operator=(Server const& other) = delete;
    Server& operator=(Server&& other) = delete;

    void start();
    void stop() noexcept;

private:
    int server_fd_;
    uint16_t const port_;
    uint8_t const max_clients_;

    bool running_{ true };

    std::unique_ptr<IEpollWrapper> epoll_;
    std::unique_ptr<ISocketWrapper> const sockwrapper_;

    std::map<std::string, std::string> g_data;

    void create_server_socket();
    void set_socket_options() const noexcept;
    void bind_socket() const;
    bool set_nonblocking(int const fd) const noexcept;
    void setup_server();

    void handle_new_connections() noexcept;
    void handle_read_event(int const client_fd);

    bool try_request(Connection& conn) noexcept;
    bool parse_req(uint8_t const* data, size_t size, std::vector<std::string>& parsed_cmds);
    bool read_cmd_length(uint8_t const*& data, uint8_t const* const end, uint32_t& out);
    bool read_cmd_data(uint8_t const*& data, uint8_t const* const end, size_t bytes_to_read, std::string& out);
    void do_request(std::vector<std::string> const& cmd, Response& resp);
    void make_response(Response& resp, std::vector<uint8_t>& out);

    void handle_write_event(int const client_fd);
    void handle_close_event(int const client_fd);
};

#endif