#ifndef EPOLL_WRAPPER_H
#define EPOLL_WRAPPER_H

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <unistd.h>    // for close(), read()
#include <sys/epoll.h> // for epoll_create1(), epoll_ctl(), struct epoll_event

enum class Flags : uint8_t
{
    WANT_READ = 1 << 0,
    WANT_WRITE = 1 << 1,
    WANT_CLOSE = 1 << 2
};

struct Connection
{
    int fd{ -1 };
    uint8_t flags{ 0 };
    std::vector<uint8_t> incoming{};
    std::vector<uint8_t> outgoing{};
};

class EpollWrapper
{
public:
    EpollWrapper(uint8_t max_events);
    ~EpollWrapper();

    EpollWrapper(EpollWrapper const& other) = delete;
    EpollWrapper(EpollWrapper&& other) = delete;
    EpollWrapper& operator=(EpollWrapper const& other) = delete;
    EpollWrapper& operator=(EpollWrapper&& other) = delete;

    void add_conn(int fd) noexcept;
    void remove_conn(int fd) noexcept;
    [[nodiscard]] int wait() noexcept;

    [[nodiscard]] auto get_event(int idx) const->epoll_event const&;
    [[nodiscard]] auto get_connection(int fd) const->Connection const&;

private:
    int epoll_fd_;
    uint8_t const max_events_;

    void add_event(int fd) noexcept;

    std::unordered_map<int, Connection> connections_;
    std::vector<epoll_event> events_;
};

#endif