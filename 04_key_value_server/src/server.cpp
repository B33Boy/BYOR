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
    epoll_->add_conn(server_fd_);

    std::cout << "Created Server, now listening on port: " << port_ << "\n";

    while (running_)
    {
        int num_events = epoll_->wait();


        for (int i = 0; i < num_events; i++)
        {
            auto& event = epoll_->get_event(i);

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

void Server::stop() noexcept
{
    running_ = false;
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
    epoll_->add_conn(client_fd);
}

/* ============================================== READ ============================================== */
void Server::handle_read_event(int const client_fd)
{
    auto& conn = epoll_->get_connection(client_fd);

    // 1. Do a non-blocking read
    std::vector<uint8_t> buf(READ_BUFFER_SIZE);
    ssize_t bytes_read = read(client_fd, buf.data(), buf.size());

    if (bytes_read == 0)
    {
        std::cout << "[DISCONNECT] Client " << client_fd << " disconnected.\n";
        handle_close_event(client_fd);
        return;
    }

    if (bytes_read < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            std::cerr << "[READ] Client " << client_fd << "-> No data available (non-blocking read), errno: " << std::strerror(errno) << "\n";
        return;
    }

    // 2. Add new data to the `Conn::incoming` buffer
    conn.incoming.insert(conn.incoming.end(), buf.begin(), buf.begin() + bytes_read);
    std::cout << "[READ] Client " << client_fd << " -> Reading " << bytes_read << " bytes\n";

    // 3. Parse requests and generate responses
    while (try_request(conn)) {}

    // 4. Remove the message from `Conn::incoming`
    if (!conn.outgoing.empty()) {
        std::cout << "[MODIFY] Client " << client_fd << " -> Outgoing buffer has data, enabling EPOLLOUT\n";
        epoll_->modify_conn(client_fd, EPOLLOUT);
        handle_write_event(client_fd); // Immediately call write event otherwise we have to wait for another loop to call it
    }
}

/* ============================================== Handle Request ============================================== */
bool Server::try_request(Connection& conn) noexcept
{
    if (conn.incoming.size() < LEN_FIELD_SIZE)
    {
        std::cout << "[ERROR] Client " << conn.fd << " -> No more bytes to be read\n";
        return false;
    }

    uint32_t data_len{};
    std::memcpy(&data_len, conn.incoming.data(), LEN_FIELD_SIZE);

    if (data_len > MAX_MSG_FIELD_SIZE)
    {
        std::cerr << "[ERROR] Client " << conn.fd << " -> given data_length greater than MAX_MSG_FIELD_SIZE, closing connection\n";
        handle_close_event(conn.fd); // TODO: NEED TO HANDLE SEGFAULT WHEN FUNCTION RETURNS and we try to access conn.fd
        return false;
    }

    if (LEN_FIELD_SIZE + data_len > conn.incoming.size())
    {
        std::cout << "Client" << conn.fd << " -> Less incoming bytes than specified in data_len \n";
        return false;
    }

    auto const* request = conn.incoming.data() + LEN_FIELD_SIZE;

    // Validate memory before creating string_view
    if (request == nullptr || data_len > conn.incoming.size() - LEN_FIELD_SIZE)
    {
        std::cerr << "[ERROR] Client " << conn.fd << " -> Invalid request pointer or length.\n";
        return false;
    }

    std::vector<std::string> cmd;
    if (!parse_req(request, data_len, cmd))
    {
        std::cout << "[ERROR] Client " << conn.fd << " -> Bad Request\n";
        return false;
    }

    Response resp{};
    do_request(cmd, resp);

    conn.incoming.erase(conn.incoming.begin(), conn.incoming.begin() + LEN_FIELD_SIZE + data_len);

    make_response(resp, conn.outgoing);

    return true;
}

bool Server::read_cmd_data(uint8_t const*& data, uint8_t const* const end, size_t bytes_to_read, std::string& out)
{
    if (data + bytes_to_read > end)
    {
        std::cout << "[ERROR] Can't read cmd data\n";
        return false;
    }

    out.assign(data, data + bytes_to_read);
    data += bytes_to_read;

    return true;
}

bool Server::read_cmd_length(uint8_t const*& data, uint8_t const* const end, uint32_t& out)
{
    if (data + LEN_FIELD_SIZE > end)
        return false;

    memcpy(&out, data, LEN_FIELD_SIZE);

    data += LEN_FIELD_SIZE; // increment ptr
    return true;
}

bool Server::parse_req(uint8_t const* data, size_t size, std::vector<std::string>& parsed_cmds)
{
    // +------+------+------+------+------+-----+------+------+
    // | nstr | len0 | cmd0 | len1 | cmd1 | ... | lenn | cmdn |
    // +------+------+------+------+------+-----+------+------+
    // Note: For [nstr, len0, cmd0], nstr would be 1
    uint8_t const* end = data + size; // End of all bytes sent

    uint32_t nstr{};
    if (!read_cmd_length(data, end, nstr))
    {
        std::cout << "[ERROR] Can't read nstr\n";
        return false;
    }

    if (nstr > MAX_CMD_ARGS)
    {
        std::cerr << "[ERROR] Parsing -> Given number of cmds is greater than MAX CMD ARGS\n";
        return false;
    }

    for (int i = 0; i < nstr; i++)
    {
        // Read cmd length
        uint32_t cmd_len{};
        if (!read_cmd_length(data, end, cmd_len))
        {
            std::cout << "[ERROR] Can't read len" << i << "\n";
            return false;
        }

        parsed_cmds.push_back(std::string());
        if (!read_cmd_data(data, end, cmd_len, parsed_cmds.back()))
        {
            std::cout << "[ERROR] Can't read cmd" << i << "\n";
            return false;
        }
    }

    return true;
}

void Server::do_request(std::vector<std::string> const& cmd, Response& resp)
{

    if (cmd.size() == 2 && cmd[0] == "get")
    {
        auto it = g_data.find(cmd[1]);
        if (it == g_data.end())
        {
            resp.status = ResponseStatus::RES_NX;
            return;
        }
        std::string const& val{ it->second };
        resp.data.assign(val.begin(), val.end());
        resp.status = ResponseStatus::RES_OK;
    }
    else if (cmd.size() == 3 && cmd[0] == "set")
    {
        g_data[cmd[1]] = cmd[2];
        std::string  const& resp_str{ cmd[1] + " set to " + cmd[2] };
        resp.data.assign(resp_str.begin(), resp_str.end());
        resp.status = ResponseStatus::RES_OK;
    }
    else if (cmd.size() == 2 && cmd[0] == "del")
    {
        g_data.erase(cmd[1]);
        resp.status = ResponseStatus::RES_OK;
    }
    else
    {
        std::cout << "Invalid command received\n";
        resp.status = ResponseStatus::RES_ERR;
    }
}

void Server::make_response(Response& resp, std::vector<uint8_t>& out)
{
    // Push resp length to outgoing buffer
    uint32_t resp_size = sizeof(resp.status) + resp.data.size();
    out.insert(out.end(), &resp_size, &resp_size + LEN_FIELD_SIZE);

    // Push error code to outgoing buffer
    out.push_back(static_cast<uint8_t>(resp.status));

    // Push resp data to outgoing buffer
    out.insert(out.end(), resp.data.begin(), resp.data.end());
}

/* ============================================== Write ============================================== */
void Server::handle_write_event(int const client_fd)
{
    auto& conn = epoll_->get_connection(client_fd);

    if (conn.outgoing.empty()) {
        std::cout << "[WRITE] Client " << client_fd << " -> No data to send, switching to EPOLLIN\n";
        epoll_->modify_conn(client_fd, EPOLLIN);
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
        epoll_->modify_conn(client_fd, EPOLLIN);
    }
    else {
        std::cout << "[MODIFY] Client " << client_fd << " -> Still data to send, keeping EPOLLOUT\n";
        epoll_->modify_conn(client_fd, EPOLLOUT);
    }
}


/* ============================================== Close ============================================== */
void Server::handle_close_event(int client_fd)
{
    epoll_->remove_conn(client_fd);
    std::cout << "[CLOSE] Successfully removed client " << client_fd << " from epoll\n";

    if (close(client_fd) == -1)
        std::cerr << "[ERROR] Failed to close client socket " << client_fd << ": " << std::strerror(errno) << "\n";
    else
        std::cout << "[CLOSE] Successfully closed client " << client_fd << "\n";

}


