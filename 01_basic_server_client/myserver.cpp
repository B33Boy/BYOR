
#include <iostream>
#include <unistd.h> // read/write
#include <sys/socket.h> // Socket functions
#include <netinet/ip.h> // sockaddr_in struct


/**
 * @brief How a server works
 *
 * 1. socket() - define communication endpoint
 * 2. bind() - bind fd to port
 * 3. listen() - mark socket to accept incoming requests
 * 4. accept() - accept connection with client
 * 5. read() - read incoming data from client
 * 6. write() - write response back to client
 */
int main()
{
    /*
    *   domain: AF_INET - IPv4 protocols
    *   type: SOCK_STREAM - sequenced, two-way, connection-based byte streams
    *   protocol: 0 - single protocol
    */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
        std::cout << "Failed to create socket. errno: " << errno << "\n";

    /*
    *   socket: fd of socket
    *   level: SOL_SOCKET - socket API level
    *   optname: SO_REUSEADDR - allow reuse of local addresses
    *   optval: 1 - pointer to buffer where options are stored
    *   optlen: sizeof(val) - size of optval buffer
    */
    int val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234); // ntohs converts ushort value from network byte order to host byte order
    addr.sin_addr.s_addr = ntohl(0); // 0.0.0.0 means listen on all network interfaces of host

    if (bind(server_fd, (const sockaddr*)&addr, sizeof(addr)) != 0)
        std::cout << "Failed to bind a sockaddr to server_fd\n";

    // listen
    listen(server_fd, SOMAXCONN);

    while (true)
    {
        struct sockaddr_in client_addr {};
        socklen_t socklen{ sizeof(client_addr) };
        int connfd = accept(server_fd, (struct sockaddr*)&client_addr, &socklen);
        if (connfd < 0)
        {
            std::cout << "Connection failed with client addr\n";
            continue; // conn error, keep alive
        }

        char rbuf[64]{};
        // n is the number of bytes read, can be within 0 (reached end of fd) and size of rbuf
        ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
        if (n < 0)
        {
            std::cout << "Cannot read from fd\n";
            continue; // read error, keep alive
        }

        if (rbuf)
        {
            std::cout << "client says: " << rbuf << "\n";

            // Write back to client
            char servebuff[]{ "world" };
            write(connfd, servebuff, sizeof(servebuff));
        }
        close(connfd);
    }

    close(server_fd);
    return 0;
}