#ifndef EPOLL_WRAPPER_H
#define EPOLL_WRAPPER_H

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <sys/epoll.h> // for epoll_create1(), epoll_ctl(), struct epoll_event
#include <unistd.h>    // for close(), read()
#include <unordered_map>
#include <vector>

enum class Flags : uint8_t
{
    WANT_READ = 1 << 0,
    WANT_WRITE = 1 << 1,
};

struct Connection
{
    int fd{ -1 };
    uint8_t flags{ 0 };
    std::vector<uint8_t> incoming{};
    std::vector<uint8_t> outgoing{};
};

class IEpollWrapper
{
public:
    virtual ~IEpollWrapper() = default;

    virtual void add_conn(int const fd) noexcept = 0;
    virtual void remove_conn(int const fd) noexcept = 0;
    virtual void modify_conn(int const fd, uint32_t const event_flags) const noexcept = 0;
    virtual int wait() noexcept = 0;

    virtual epoll_event& get_event(int idx) = 0;
    virtual Connection& get_connection(int fd) = 0;
};

class EpollWrapper : public IEpollWrapper
{
public:
    EpollWrapper(uint8_t max_events);
    ~EpollWrapper();

    EpollWrapper(EpollWrapper const& other) = delete;
    EpollWrapper(EpollWrapper&& other) = delete;
    EpollWrapper& operator=(EpollWrapper const& other) = delete;
    EpollWrapper& operator=(EpollWrapper&& other) = delete;

    void add_conn(int const fd) noexcept override;
    void remove_conn(int const fd) noexcept override;
    void modify_conn(int const fd, uint32_t const event_flags) const noexcept override;
    [[nodiscard]] int wait() noexcept override;

    [[nodiscard]] epoll_event& get_event(int const idx) override;
    [[nodiscard]] Connection& get_connection(int const fd) override;

private:
    int epoll_fd_;
    uint8_t const max_events_;

    void monitor(int fd) const noexcept;
    void unmonitor(int fd) const noexcept;

    std::unordered_map<int, Connection> connections_;
    std::vector<epoll_event> events_;
};

#endif