#ifndef EPOLL_WRAPPER_H
#define EPOLL_WRAPPER_H

#include "spdlog/spdlog.h"

#include <cstdint>
#include <stdexcept>
#include <sys/epoll.h> // for epoll_create1(), epoll_ctl(), struct epoll_event
#include <unistd.h>    // for close(), read()
#include <unordered_map>
#include <vector>

struct Connection
{
    int fd{ -1 };
    std::vector<uint8_t> incoming{};
    std::vector<uint8_t> outgoing{};
};

// ========================== CTRP BASE ==========================
template <typename Derived>
class IEpollWrapperBase
{
public:
    void add_conn(int const fd) noexcept
    {
        derived().add_conn_impl(fd);
    }

    void remove_conn(int const fd) noexcept
    {
        derived().remove_conn_impl(fd);
    }

    void modify_conn(int const fd, uint32_t const event_flags) const noexcept
    {
        derived().modify_conn_impl(fd, event_flags);
    }

    [[nodiscard]] int wait() noexcept
    {
        return derived().wait_impl();
    }

    [[nodiscard]] epoll_event& get_event(int const idx)
    {
        return derived().get_event_impl(idx);
    }

    [[nodiscard]] Connection& get_connection(int const fd)
    {
        return derived().get_connection_impl(fd);
    }

private:
    IEpollWrapperBase() = default; // prevent direct instantiation
    friend Derived;                // Allow only derived to construct base

    // Simplify syntax for delegating work to derived class
    Derived& derived()
    {
        return static_cast<Derived&>(*this);
    }

    Derived const& derived() const noexcept
    {
        return static_cast<const Derived&>(*this);
    }
};

// ========================== CTRP DERIVED ==========================
class EpollWrapper final : public IEpollWrapperBase<EpollWrapper>
{
public:
    EpollWrapper(EpollWrapper const& other) = delete;
    EpollWrapper(EpollWrapper&& other) noexcept = default;
    EpollWrapper& operator=(EpollWrapper const& other) = delete;
    EpollWrapper& operator=(EpollWrapper&& other) noexcept = default;

    EpollWrapper(uint8_t max_events) : max_events_(max_events), events_(max_events)
    {
        epoll_fd_ = epoll_create1(0);
        if ( epoll_fd_ == -1 )
            throw std::runtime_error("Failed to create epoll fd");
    }
    ~EpollWrapper()
    {
        // Close all monitoring connections server + clients
        for ( auto& [fd, _] : connections_ )
            remove_conn(fd);

        // Close epoll fd
        if ( close(epoll_fd_) != 0 )
            spdlog::error("Failed to close epoll fd. err: {}", std::strerror(errno));
    }

    void add_conn_impl(int const fd) noexcept
    {
        if ( connections_.find(fd) != connections_.end() )
            return;

        Connection conn{};
        conn.fd = fd;
        connections_[fd] = conn;

        monitor(fd);
    }

    void remove_conn_impl(int const fd) noexcept
    {
        if ( connections_.find(fd) == connections_.end() )
            return;

        connections_.erase(fd);
        unmonitor(fd);
    }

    void modify_conn_impl(int const fd, uint32_t const event_flags) const noexcept
    {
        epoll_event event{};
        event.events = event_flags;
        event.data.fd = fd;

        epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
    }

    [[nodiscard]] int wait_impl() noexcept
    {
        return epoll_wait(epoll_fd_, events_.data(), max_events_, -1);
    }

    [[nodiscard]] epoll_event& get_event_impl(int const idx)
    {
        return events_.at(idx);
    }

    [[nodiscard]] Connection& get_connection_impl(int const fd)
    {
        auto it = connections_.find(fd);
        if ( it == connections_.end() )
            throw std::out_of_range("Connection not found");
        return it->second;
    }

private:
    int epoll_fd_;
    uint8_t const max_events_;

    std::unordered_map<int, Connection> connections_;
    std::vector<epoll_event> events_;

    void monitor(int fd) const noexcept
    {
        epoll_event event{};
        event.events = EPOLLIN;
        event.data.fd = fd;

        if ( epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) )
            spdlog::error("Failed to add fd: {} to epoll. err: {}", fd, std::strerror(errno));
    }

    void unmonitor(int fd) const noexcept
    {
        if ( epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) )
            spdlog::error("Failed to remove fd: {} from epoll. err: {}", fd, std::strerror(errno));
    }
};

#endif