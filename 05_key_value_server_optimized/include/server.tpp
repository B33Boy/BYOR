#include "server.h"

template <class ISocketWrapperBase, class IEpollWrapperBase>
void Server<ISocketWrapperBase, IEpollWrapperBase>::create_server_socket()
{
    server_fd_ = sockwrapper_.socket(AF_INET, SOCK_STREAM, 0);
    if ( server_fd_ == -1 )
        throw std::runtime_error("Failed to create socket");
}

template <class ISocketWrapperBase, class IEpollWrapperBase>
void Server<ISocketWrapperBase, IEpollWrapperBase>::set_socket_options() const noexcept
{
    sockwrapper_.setsockopt(server_fd_);
}

template <class ISocketWrapperBase, class IEpollWrapperBase>
void Server<ISocketWrapperBase, IEpollWrapperBase>::bind_socket() const
{
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ntohl(0);
    addr.sin_port = ntohs(port_);

    if ( sockwrapper_.bind(server_fd_, (const sockaddr*)&addr, sizeof(addr)) == -1 )
        throw std::runtime_error("Failed to bind a sockaddr to server_fd");
}

template <class ISocketWrapperBase, class IEpollWrapperBase>
bool Server<ISocketWrapperBase, IEpollWrapperBase>::set_nonblocking(int const fd) const noexcept
{
    int flags = sockwrapper_.fcntl(fd, F_GETFL);
    if ( flags == -1 )
    {
        spdlog::error("Failed to GET fcntl(). err: {}", std::strerror(errno));
        return false;
    }

    if ( sockwrapper_.fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1 )
    {
        spdlog::error("Failed to SET fcntl(). err: {}", std::strerror(errno));
        return false;
    }
    return true;
}

template <class ISocketWrapperBase, class IEpollWrapperBase>
void Server<ISocketWrapperBase, IEpollWrapperBase>::setup_server()
{
    try
    {
        create_server_socket();
        set_socket_options();
        bind_socket();
        if ( !set_nonblocking(server_fd_) )
            throw std::runtime_error("Failed to set server socket as nonblocking");
        listen(server_fd_, max_clients_);
    }
    catch ( std::runtime_error const& e )
    {
        spdlog::error("Runtime exception while setting up server! {}", e.what());
        sockwrapper_.close(server_fd_);
        throw;
    }
}

template <class ISocketWrapperBase, class IEpollWrapperBase>
void Server<ISocketWrapperBase, IEpollWrapperBase>::start()
{
    setup_server();

    // Add server sock to epoll_
    epoll_.add_conn(server_fd_);
    spdlog::info("Created Server, now listening on port: {}", port_);

    while ( running_ )
    {
        int num_events = epoll_.wait();
        spdlog::info("Number of ready events: {}", num_events);

        for ( int i = 0; i < num_events; i++ )
        {
            auto& event = epoll_.get_event(i);

            if ( event.data.fd == server_fd_ )
                handle_new_connections();
            else
            {
                auto& conn = epoll_.get_connection(event.data.fd);

                if ( event.events & EPOLLIN )
                    handle_read_event(conn);

                if ( event.events & EPOLLOUT )
                    handle_write_event(conn);

                if ( event.events & EPOLLERR || event.events & EPOLLHUP )
                    handle_close_event(conn);
            }

            spdlog::info("=========================================================");
        }
    }
}

template <class ISocketWrapperBase, class IEpollWrapperBase>
void Server<ISocketWrapperBase, IEpollWrapperBase>::stop() noexcept
{
    running_ = false;
}

/* ============================================== New Conn ============================================== */
template <class ISocketWrapperBase, class IEpollWrapperBase>
void Server<ISocketWrapperBase, IEpollWrapperBase>::handle_new_connections() noexcept
{
    sockaddr_in client_addr{};
    socklen_t socklen{ sizeof(client_addr) };
    int client_fd = sockwrapper_.accept(server_fd_, (sockaddr*)&client_addr, &socklen);

    if ( client_fd == -1 )
    {
        spdlog::error("[ERROR] Connection failed with client. err: {}", std::strerror(errno));
        return;
    }

    spdlog::info("[INFO] New client connected: {}", client_fd);

    if ( !set_nonblocking(client_fd) )
    {
        spdlog::error("[ERROR] Failed to set client socket as non-blocking. err: {}", std::strerror(errno));
        close(client_fd);
        return;
    }
    epoll_.add_conn(client_fd);
}

/* ============================================== READ ============================================== */
template <class ISocketWrapperBase, class IEpollWrapperBase>
void Server<ISocketWrapperBase, IEpollWrapperBase>::handle_read_event(Connection& conn)
{
    // 1. Do a non-blocking read
    std::vector<uint8_t> buf(READ_BUFFER_SIZE);
    ssize_t bytes_read = read(conn.fd, buf.data(), buf.size());

    if ( bytes_read == 0 )
    {
        spdlog::info("[DISCONNECT] Client {} disconnected", conn.fd);
        handle_close_event(conn);
        return;
    }

    if ( bytes_read < 0 )
    {
        if ( errno == EAGAIN || errno == EWOULDBLOCK )
            spdlog::error("[READ] Client {} -> No data available (non-blocking read). err: {}", conn.fd,
                          std::strerror(errno));

        return;
    }

    // 2. Add new data to the `Conn::incoming` buffer
    conn.incoming.insert(conn.incoming.end(), buf.begin(), buf.begin() + bytes_read);
    spdlog::info("[READ] Client: {} -> Reading {} bytes", conn.fd, bytes_read);

    // 3. Parse requests and generate responses
    while ( try_request(conn) )
    {
    }

    // 4. Remove the message from `Conn::incoming` by calling write
    if ( !conn.outgoing.empty() )
    {
        spdlog::info("[MODIFY] Client {} -> Outgoing buffer has data, enabling EPOLLOUT", conn.fd);
        handle_write_event(conn);
    }
}

/* ============================================== Handle Request ============================================== */
template <class ISocketWrapperBase, class IEpollWrapperBase>
bool Server<ISocketWrapperBase, IEpollWrapperBase>::try_request(Connection& conn) noexcept
{
    if ( conn.incoming.size() < LEN_FIELD_SIZE )
    {
        spdlog::info("[ERROR] Client {} -> No more bytes to be read", conn.fd);
        return false;
    }

    uint32_t data_len{};
    std::memcpy(&data_len, conn.incoming.data(), LEN_FIELD_SIZE);

    spdlog::info("[READ] Client {} -> Request is comprised of {} bytes", conn.fd, data_len);

    if ( data_len > MAX_MSG_FIELD_SIZE )
    {
        spdlog::error("[ERROR] Client {} -> given data_length greater than MAX_MSG_FIELD_SIZE, closing connection",
                      conn.fd);

        handle_close_event(conn); // TODO: NEED TO HANDLE SEGFAULT WHEN FUNCTION RETURNS and we try to access conn.fd
        return false;
    }

    if ( LEN_FIELD_SIZE + data_len > conn.incoming.size() )
    {
        spdlog::info("[READ] Client {} -> Less incoming bytes than specified in data_len", conn.fd);
        return false;
    }

    auto const* request = conn.incoming.data() + LEN_FIELD_SIZE;

    if ( !request )
    {
        spdlog::error("[ERROR] Client {} -> Invalid request pointer or length.", conn.fd);
        return false;
    }

    std::vector<std::string> cmd;
    if ( !parse_req(request, data_len, cmd) )
    {
        spdlog::info("[ERROR] Client {} -> Bad Request", conn.fd);
        return false;
    }

    Response resp{};
    do_request(cmd, resp);

    conn.incoming.erase(conn.incoming.begin(), conn.incoming.begin() + LEN_FIELD_SIZE + data_len);

    make_response(resp, conn.outgoing);

    return true;
}

template <class ISocketWrapperBase, class IEpollWrapperBase>
bool Server<ISocketWrapperBase, IEpollWrapperBase>::read_cmd_data(uint8_t const*& data, uint8_t const* const end,
                                                                  size_t bytes_to_read, std::string& out)
{
    if ( data + bytes_to_read > end )
    {
        spdlog::info("[ERROR] Can't read cmd data");
        return false;
    }

    out.assign(data, data + bytes_to_read);
    data += bytes_to_read;

    return true;
}

template <class ISocketWrapperBase, class IEpollWrapperBase>
bool Server<ISocketWrapperBase, IEpollWrapperBase>::read_cmd_length(uint8_t const*& data, uint8_t const* const end,
                                                                    uint32_t& out)
{
    if ( data + LEN_FIELD_SIZE > end )
        return false;

    memcpy(&out, data, LEN_FIELD_SIZE);

    data += LEN_FIELD_SIZE; // increment ptr
    return true;
}

template <class ISocketWrapperBase, class IEpollWrapperBase>
bool Server<ISocketWrapperBase, IEpollWrapperBase>::parse_req(uint8_t const* data, size_t size,
                                                              std::vector<std::string>& parsed_cmds)
{
    // +------+------+------+------+------+-----+------+------+
    // | nstr | len0 | cmd0 | len1 | cmd1 | ... | lenn | cmdn |
    // +------+------+------+------+------+-----+------+------+
    // Note: For [nstr, len0, cmd0], nstr would be 1
    uint8_t const* end = data + size; // End of all bytes sent

    uint32_t nstr{};
    if ( !read_cmd_length(data, end, nstr) )
    {
        spdlog::info("[ERROR] Can't read nstr");
        return false;
    }

    if ( nstr > MAX_CMD_ARGS )
    {
        spdlog::error("[ERROR] Given number of cmds is greater than MAX CMD ARGS");
        return false;
    }

    for ( int i = 0; i < nstr; i++ )
    {
        // Read cmd length
        uint32_t cmd_len{};
        if ( !read_cmd_length(data, end, cmd_len) )
        {
            spdlog::info("[ERROR] Can't read len {}", i);
            return false;
        }

        parsed_cmds.push_back(std::string());
        if ( !read_cmd_data(data, end, cmd_len, parsed_cmds.back()) )
        {
            spdlog::info("[ERROR] Can't read cmd");
            return false;
        }
    }

    return true;
}

template <class ISocketWrapperBase, class IEpollWrapperBase>
void Server<ISocketWrapperBase, IEpollWrapperBase>::do_request(std::vector<std::string> const& cmd, Response& resp)
{

    if ( cmd.size() == 2 && cmd[0] == "get" )
    {
        auto it = g_data.find(cmd[1]);
        if ( it == g_data.end() )
        {
            resp.status = ResponseStatus::RES_NX;
            return;
        }
        std::string const& val{ it->second };
        resp.data.assign(val.begin(), val.end());
        resp.status = ResponseStatus::RES_OK;
    }
    else if ( cmd.size() == 3 && cmd[0] == "set" )
    {
        g_data[cmd[1]] = cmd[2];
        std::string const& resp_str{ cmd[1] + " set to " + cmd[2] };
        resp.data.assign(resp_str.begin(), resp_str.end());
        resp.status = ResponseStatus::RES_OK;
    }
    else if ( cmd.size() == 2 && cmd[0] == "del" )
    {
        g_data.erase(cmd[1]);
        resp.status = ResponseStatus::RES_OK;
    }
    else
    {
        spdlog::info("[ERROR]Invalid command received");
        resp.status = ResponseStatus::RES_ERR;
    }
}

template <class ISocketWrapperBase, class IEpollWrapperBase>
void Server<ISocketWrapperBase, IEpollWrapperBase>::make_response(Response& resp, std::vector<uint8_t>& out)
{
    // Push resp length to outgoing buffer
    uint32_t resp_size = sizeof(resp.status) + resp.data.size();
    uint8_t* size_ptr = reinterpret_cast<uint8_t*>(&resp_size);
    out.insert(out.end(), size_ptr, size_ptr + sizeof(resp_size));

    // Push error code to outgoing buffer
    out.push_back(static_cast<uint8_t>(resp.status));

    // Push resp data to outgoing buffer
    out.insert(out.end(), resp.data.begin(), resp.data.end());
}

/* ============================================== Write ============================================== */
template <class ISocketWrapperBase, class IEpollWrapperBase>
void Server<ISocketWrapperBase, IEpollWrapperBase>::handle_write_event(Connection& conn)
{
    if ( conn.outgoing.empty() )
    {
        spdlog::info("[WRITE] Client {} -> No data to send, switching to EPOLLIN", conn.fd);
        epoll_.modify_conn(conn.fd, EPOLLIN);
        return;
    }

    ssize_t bytes_written = write(conn.fd, conn.outgoing.data(), conn.outgoing.size());

    if ( bytes_written < 0 )
    {
        spdlog::error("[ERROR] Write error to client {} -> err: {}", conn.fd, std::strerror(errno));
        if ( errno == EAGAIN || errno == EWOULDBLOCK )
            return;
        handle_close_event(conn);
        return;
    }

    spdlog::info("[WRITE] Client {} -> Wrote {} bytes", conn.fd, bytes_written);
    conn.outgoing.erase(conn.outgoing.begin(), conn.outgoing.begin() + bytes_written);

    if ( conn.outgoing.empty() )
    {
        spdlog::info("[MODIFY] Client {} -> No more data to read, switching to EPOLLIN", conn.fd);
        epoll_.modify_conn(conn.fd, EPOLLIN);
    }
    else
    {
        spdlog::info("[MODIFY] Client {} -> Still have data to send, keeping EPOLLOUT", conn.fd);
    }
}

/* ============================================== Close ============================================== */
template <class ISocketWrapperBase, class IEpollWrapperBase>
void Server<ISocketWrapperBase, IEpollWrapperBase>::handle_close_event(Connection& conn)
{
    int fd = conn.fd;

    spdlog::info("[CLOSE] Removing client {} from epoll", fd);
    epoll_.remove_conn(fd);

    if ( close(fd) == -1 )
        spdlog::error("[ERROR] Failed to close client {}'s socket -> err: {}", fd, std::strerror(errno));
    else
        spdlog::info("[CLOSE] Successfully closed client {}", fd);
}
