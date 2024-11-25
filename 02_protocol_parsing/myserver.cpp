#include <iostream>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>


const size_t k_max_msg = 4096;

/**
 * @brief reads up to n bytes in the kernel
 *
 * @param fd fd of server
 * @param buf pointer to char arr
 * @param n amount of bytes we want to read
 * @return int32_t 0
 */
static int32_t read_full(int fd, char* buf, size_t n)
{
    /**
     * The read() syscall just returns whatever data is available in the kernel, or blocks if there is none.
     * It’s up to the application to handle insufficient data.
     * The read_full() function reads from the kernel until it gets exactly n bytes.
     */
    while (n > 0)
    {
        ssize_t rv = read(fd, buf, n);

        if (rv <= 0)
        {
            return -1;
        }

        assert((size_t)rv <= n); // whatever we receive must not be gt or equal to n        
        n -= (size_t)rv;
        buf += rv; // increment buffer pointer so that we can receive the next set of bytes 
    }
    return 0;
}

/**
 * @brief writes data
 *
 * @param fd fd of server
 * @param buf pointer to char arr
 * @param n amount of bytes we want to write
 * @return int32_t 0
 */
static int32_t write_all(int fd, const char* buf, size_t n)
{
    /**
     * The write() syscall may return successfully with partial data written if the kernel buffer is full,
     * we must keep trying when the write() returns fewer bytes than we need.
     */
    while (n > 0)
    {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0)
        {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}


/**
 * @brief handles one client connection at once
 *
 * @param connfd
 * @return int32_t
 */
static int32_t one_request(int connfd)
{
    // 4 bytes header
    // First 4 bytes contain length of request, k_max_msg is the actual request message, +1 for null terminator
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err)
    {
        if (errno == 0) { std::cout << "EOF\n"; }
        else { std::cout << "read() error\n"; }
        return err;
    }

    uint32_t len{ 0 }; // length of request obtained from header
    memcpy(&len, rbuf, 4); // copy 4 bytes (little endian) from buffer to len
    if (len > k_max_msg) { std::cout << "Message too long\n"; }

    err = read_full(connfd, &rbuf[4], len); // read from 4th byte onwards

    if (err) { std::cout << "read() error"; return err; }

    // do something
    rbuf[4 + len] = '\0';
    std::cout << "client says: " << &rbuf[4];

    // reply using same protocol
    const char reply[]{ "world \n" };
    len = (uint32_t)strlen(reply);
    char wbuf[4 + sizeof(reply)];

    memcpy(wbuf, &len, 4); // copy length to buffer
    memcpy(&wbuf[4], reply, len); // copy the actual message into buffer

    return write_all(connfd, wbuf, 4 + len);
}


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
    {
        std::cout << "Failed to create socket. errno: " << errno << "\n";
    }

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
    {
        std::cout << "Failed to bind a sockaddr to server_fd\n";
    }

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

        while (true)
        {
            int32_t err = one_request(connfd);
            if (err) break;
        }

        close(connfd);
    }

    close(server_fd);
    return 0;
}