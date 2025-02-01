#include "server.h"

void Server::create_server_socket()
{
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1)
        throw std::runtime_error("Failed to create socket");
}

void Server::set_socket_options()
{
    int val = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
}

void Server::bind_socket()
{
    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ntohl(0);
    addr.sin_port = ntohs(port_);


    if (bind(server_fd_, (const sockaddr*)&addr, sizeof(addr)) == -1)
        throw std::runtime_error("Failed to bind a sockaddr to server_fd");
}

bool Server::set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1)
    {
        std::cerr << "Failed to GET fcntl():" << errno << "\n";
        return false;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        std::cerr << "Failed to SET fcntl():" << errno << "\n";
        return false;
    }
    return true;
}

void Server::setup_server()
{
    try
    {
        create_server_socket();
        set_socket_options();
        bind_socket();
        set_nonblocking(server_fd_); // set server socket as nonblocking, TODO: Handle fail case
        listen(server_fd_, max_clients_);
    }
    catch (std::runtime_error const& e)
    {
        std::cerr << e.what() << std::endl;
        close(server_fd_);
    }
}

void Server::start()
{
    setup_server();

    // Add server sock to epoll_
    epoll_.add_conn(server_fd_);

    std::cout << "Created Server, now listening on port: " << port_ << "\n";

    while (true)
    {
        int num_events = epoll_.wait();

        for (int i = 0; i < num_events; i++)
        {
            auto& event = epoll_.get_event(i);
            if (event.data.fd == server_fd_)
                handle_new_connections();

            if (event.events & EPOLLIN)
                handle_read_event(event.data.fd);

            if (event.events & EPOLLOUT)
                handle_write_event(event.data.fd);

            if (event.events & EPOLLERR || event.events & EPOLLHUP)
                handle_close_event(event.data.fd);
        }
    }
}

void Server::handle_new_connections()
{
    sockaddr_in client_addr{};
    socklen_t socklen{ sizeof(client_addr) };
    int client_fd = accept(server_fd_, (sockaddr*)&client_addr, &socklen);
    if (client_fd == -1)
    {
        std::cerr << "Connection failed with client:" << errno << "\n";
    }

    set_nonblocking(client_fd);
    epoll_.add_conn(client_fd);
}

void Server::handle_read_event(int client_fd)
{
    auto conn = epoll_.get_connection(client_fd);

    std::vector<uint8_t> temp_buffer(1024);
    ssize_t bytes_read = read(client_fd, temp_buffer.data(), temp_buffer.size());

    std::cout << "Read " << bytes_read << " bytes from client " << client_fd << "\n";

    if (bytes_read <= 0)
    {
        if (bytes_read == 0)
            std::cout << "Client " << client_fd << " disconnected.\n";
        else
            std::cerr << "read() error\n";

        conn.flags |= static_cast<uint8_t>(Flags::WANT_CLOSE);
        return;
    }

    conn.incoming.insert(conn.incoming.end(), temp_buffer.begin(), temp_buffer.begin() + bytes_read);

    // If there is any data in outgoing buffer, toggle WANT_WRITE flag 
    if (!conn.outgoing.empty())
    {
        conn.flags |= static_cast<uint8_t>(Flags::WANT_WRITE);
        conn.flags &= ~static_cast<uint8_t>(Flags::WANT_READ);
    }
}

void Server::handle_write_event(int client_fd)
{
    auto conn = epoll_.get_connection(client_fd);
    if (!conn.outgoing.empty())
    {
        ssize_t bytes_written = write(client_fd, conn.outgoing.data(), conn.outgoing.size());
        if (bytes_written < 0)
        {
            std::cerr << "write() error\n";
            conn.flags |= static_cast<uint8_t>(Flags::WANT_CLOSE);
            return;
        }

        conn.outgoing.erase(conn.outgoing.begin(), conn.outgoing.begin() + bytes_written);

        if (conn.outgoing.empty())
        {
            conn.flags |= static_cast<uint8_t>(Flags::WANT_READ);
            conn.flags &= ~static_cast<uint8_t>(Flags::WANT_WRITE);
        }
    }
}

void Server::handle_close_event(int client_fd)
{
    auto conn = epoll_.get_connection(client_fd);
    if (conn.flags & static_cast<uint8_t>(Flags::WANT_CLOSE))
    {
        close(client_fd);
        epoll_.remove_conn(client_fd);
    }
}


