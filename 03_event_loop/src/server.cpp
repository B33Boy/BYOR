#include "server.h"

void Server::create_server_socket()
{
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1)
        throw std::runtime_error("Failed to create socket");
}

void Server::set_socket_options() const noexcept
{
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

void Server::bind_socket() const
{
    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ntohl(0);
    addr.sin_port = ntohs(port_);


    if (bind(server_fd_, (const sockaddr*)&addr, sizeof(addr)) == -1)
        throw std::runtime_error("Failed to bind a sockaddr to server_fd");
}

bool Server::set_nonblocking(int const fd) const noexcept
{
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1)
    {
        std::cerr << "Failed to GET fcntl():" << std::strerror(errno) << "\n";
        return false;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        std::cerr << "Failed to SET fcntl():" << std::strerror(errno) << "\n";
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
        if (!set_nonblocking(server_fd_))
            throw std::runtime_error("Failed to set server socket as nonblocking");
        listen(server_fd_, max_clients_);
    }
    catch (std::runtime_error const& e)
    {
        std::cerr << e.what() << std::endl;
        close(server_fd_);
        throw;
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

            if (event.data.fd != server_fd_ && event.events & EPOLLIN)
                handle_read_event(event.data.fd);

            if (event.data.fd != server_fd_ && event.events & EPOLLOUT)
                handle_write_event(event.data.fd);

            if (event.data.fd != server_fd_ && (event.events & EPOLLERR || event.events & EPOLLHUP))
                handle_close_event(event.data.fd);

            std::cout << "=========================================================" << "\n";
        }
    }
}

/* ============================================== New Conn ============================================== */
void Server::handle_new_connections() noexcept
{
    sockaddr_in client_addr{};
    socklen_t socklen{ sizeof(client_addr) };
    int client_fd = accept(server_fd_, (sockaddr*)&client_addr, &socklen);

    if (client_fd == -1) {
        std::cerr << "[ERROR] Connection failed with client: " << std::strerror(errno) << "\n";
        return;
    }

    std::cout << "[INFO] New client connected: " << client_fd << "\n";

    if (!set_nonblocking(client_fd)) {
        std::cerr << "[ERROR] Failed to set client socket as non-blocking: " << std::strerror(errno) << "\n";
        close(client_fd);
        return;
    }
    epoll_.add_conn(client_fd);
}

/* ============================================== READ ============================================== */
void Server::handle_read_event(int const client_fd)
{
    auto& conn = epoll_.get_connection(client_fd);

    std::vector<uint8_t> temp_buffer(1024);
    ssize_t bytes_read = read(client_fd, temp_buffer.data(), temp_buffer.size());

    if (bytes_read == 0)
    {
        std::cout << "[DISCONNECT] Client " << client_fd << " disconnected.\n";
        handle_close_event(client_fd);
        return;
    }

    if (bytes_read < 0)
    {
        // std::cerr << "[ERROR] Read error from client " << client_fd << ", errno: " << std::strerror(errno) << "\n";
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            std::cerr << "[ERROR] Nonblocking client " << client_fd << " has no data to be read, errno: " << std::strerror(errno) << "\n";
        return;
    }

    conn.incoming.insert(conn.incoming.end(), temp_buffer.begin(), temp_buffer.begin() + bytes_read);
    std::cout << "[READ] Client " << client_fd << " -> Reading " << bytes_read << " bytes\n";

    // ECHO SERVER BEHAVIOUR
    conn.outgoing.insert(conn.outgoing.end(), conn.incoming.begin(), conn.incoming.end());
    conn.incoming.clear();

    if (!conn.outgoing.empty()) {
        std::cout << "[MODIFY] Client " << client_fd << " -> Enabling EPOLLOUT\n";
        epoll_.modify_conn(client_fd, EPOLLOUT);
        handle_write_event(client_fd); // Immediately call write event otherwise we have to wait for another loop to call it
    }
}


/* ============================================== Write ============================================== */
void Server::handle_write_event(int const client_fd)
{
    auto& conn = epoll_.get_connection(client_fd);

    if (conn.outgoing.empty()) {
        std::cout << "[WRITE] Client " << client_fd << " -> No data to send, switching to EPOLLIN\n";
        epoll_.modify_conn(client_fd, EPOLLIN);
        return;
    }

    ssize_t bytes_written = write(client_fd, conn.outgoing.data(), conn.outgoing.size());

    if (bytes_written < 0) {
        std::cerr << "[ERROR] Write error to client " << client_fd << ", errno: " << std::strerror(errno) << "\n";
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        handle_close_event(client_fd);
        return;
    }

    std::cout << "[WRITE] Client " << client_fd << " -> Wrote " << bytes_written << " bytes\n";
    conn.outgoing.erase(conn.outgoing.begin(), conn.outgoing.begin() + bytes_written);

    if (conn.outgoing.empty()) {
        std::cout << "[MODIFY] Client " << client_fd << " -> No more data to read, switching to EPOLLIN\n";
        epoll_.modify_conn(client_fd, EPOLLIN);
    }
    else {
        std::cout << "[MODIFY] Client " << client_fd << " -> Still data to send, keeping EPOLLOUT\n";
        epoll_.modify_conn(client_fd, EPOLLOUT);
    }
}

int Server::read_full(int const fd, std::vector<uint8_t>& buf, size_t const num_to_read)
{
    int bytes_read{};

    while (bytes_read < num_to_read)
    {
        int num_read = read(fd, buf.data() + bytes_read, num_to_read - bytes_read);
        if (num_read < 0)
            return -1;

        bytes_read += num_read;
    }
    return 0;
}

int Server::write_all(int fd, std::vector<uint8_t>& buf, size_t num_to_write)
{
    int bytes_written{};

    while (bytes_written < num_to_write)
    {
        int num_written = write(fd, buf.data() + bytes_written, num_to_write - bytes_written);
        if (num_written < 0)
            return -1;

        bytes_written += num_written;
    }
    return 0;
}

/* ============================================== Close ============================================== */
void Server::handle_close_event(int client_fd)
{
    epoll_.remove_conn(client_fd);
    std::cout << "[CLOSE] Successfully removed client " << client_fd << " from epoll\n";

    if (close(client_fd) == -1)
        std::cerr << "[ERROR] Failed to close client socket " << client_fd << ": " << std::strerror(errno) << "\n";
    else
        std::cout << "[CLOSE] Successfully closed client " << client_fd << "\n";

}


