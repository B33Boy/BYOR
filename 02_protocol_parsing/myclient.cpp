#include <iostream>

#include <assert.h>
#include <cstring> // strlen
#include <unistd.h> // read/write
#include <sys/socket.h> // Socket functions
#include <netinet/ip.h> // sockaddr_in struct

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

static int32_t query(int fd, const char* text)
{
    uint32_t len = std::strlen(text);

    if (len > k_max_msg) { return -1; }

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], text, len);

    if (int32_t err = write_all(fd, wbuf, 4 + len)) { return err; }

    char rbuf[4 + k_max_msg + 1];
    errno = 0;

    int32_t err = read_full(fd, rbuf, 4);

    if (err)
    {
        if (errno == 0) { std::cout << "EOF\n"; }
        else { std::cout << "read() error\n"; }
        return err;
    }

    // Header
    memcpy(&len, rbuf, 4); // assume little endian
    if (len > k_max_msg) { std::cout << "too long\n"; }

    // Body
    err = read_full(fd, &rbuf[4], len);
    if (err) { std::cout << "read error\n"; }

    // Some reply
    rbuf[4 + len] = '\0';

    std::cout << "server says " << &rbuf[4];

    return 0;
}

/**
 * @brief How a client works
 *
 * 1. socket() - define communication endpoint
 * 2. connect() - connect socket to an addr specified by a fd
 * 3. write() - send data to server
 * 4. read() - read incoming data from server
 */
int main()
{
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (client_fd < 0) { std::cout << "Failed to create socket. errno: " << errno << "\n"; }

    struct sockaddr_in server_addr {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = ntohs(1234);
    // INADDR_LOOPBACK means only clients on the same machine can connect to it
    server_addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);

    if (connect(client_fd, (const struct sockaddr*)&server_addr, sizeof(server_addr)) != 0)
    {
        std::cout << "Failed to connect to server\n";
    }

    auto err = query(client_fd, "hello1 \n");
    if (err) { close(client_fd); return 0; }

    err = query(client_fd, "hello2 \n");
    if (err) { close(client_fd); return 0; }

    err = query(client_fd, "hello3 \n");
    if (err) { close(client_fd); return 0; }

    return 0;
}