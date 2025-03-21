#include "epollwrapper.h"

EpollWrapper::EpollWrapper(uint8_t max_events) : max_events_(max_events), events_(max_events)
{
    epoll_fd_ = epoll_create1(0);
    if ( epoll_fd_ == -1 )
        throw std::runtime_error("Failed to create epoll fd");
}

EpollWrapper::~EpollWrapper()
{
    // Close all monitoring connections
    for ( auto& [fd, _] : connections_ )
        remove_conn(fd);

    // Close epoll fd
    if ( close(epoll_fd_) != 0 )
        std::cerr << "Failed to close epoll fd. errno: " << errno << "\n";
};

void EpollWrapper::add_conn(int const fd) noexcept
{
    if ( connections_.find(fd) != connections_.end() )
        return;

    Connection conn{};
    conn.fd = fd;
    conn.flags |= static_cast<uint8_t>(Flags::WANT_READ);
    connections_[fd] = conn;

    monitor(fd);
}

void EpollWrapper::monitor(int const fd) const noexcept
{
    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = fd;

    if ( epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) )
        std::cerr << "Failed to add fd " << fd << " to epoll. errno: " << errno << "\n";
}

void EpollWrapper::remove_conn(int const fd) noexcept
{
    if ( connections_.find(fd) == connections_.end() )
        return;

    connections_.erase(fd);
    unmonitor(fd);
}

void EpollWrapper::unmonitor(int const fd) const noexcept
{
    if ( epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) )
        std::cerr << "Failed to remove fd " << fd << " from epoll. errno: " << errno << "\n";
}

void EpollWrapper::modify_conn(int const fd, uint32_t const event_flags) const noexcept
{
    epoll_event event{};
    event.events = event_flags;
    event.data.fd = fd;

    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
}

[[nodiscard]] int EpollWrapper::wait() noexcept
{
    int event_count = epoll_wait(epoll_fd_, events_.data(), max_events_, -1);
    // std::cout << "Number of ready events: " << event_count << '\n';
    return event_count;
}

[[nodiscard]] auto EpollWrapper::get_event(int const idx) -> epoll_event&
{
    return events_.at(idx);
}

[[nodiscard]] auto EpollWrapper::get_connection(int const fd) -> Connection&
{
    auto it = connections_.find(fd);
    if ( it == connections_.end() )
        throw std::out_of_range("Connection not found");
    // std::cerr << "Connection not found\n";
    return it->second;
}