#include <array>      // std::array
#include <cstdint>    // uint32_t, int32_t
#include <cstring>    // std::memcpy, std::strlen
#include <iostream>   // std::cout

#include <arpa/inet.h>  // ntohs(), ntohl()
#include <errno.h>      // errno
#include <netinet/ip.h> // sockaddr_in
#include <sys/socket.h> // socket(), setsockopt(), bind(), listen(), accept()
#include <unistd.h>     // close(), read(), write()
#include <assert.h>

constexpr size_t LEN_FIELD_SIZE{ 4 };
constexpr size_t MAX_MSG_FIELD_SIZE{ 16 };

int32_t read_full(int fd, char* buf, size_t num_to_read)
{
    int bytes_read{};

    while (bytes_read < num_to_read)
    {
        int num_read = read(fd, buf + bytes_read, num_to_read - bytes_read);
        if (num_read <= 0)
            return -1;

        bytes_read += num_read;
    }
    return 0;
}

int32_t write_all(int fd, char const* buf, size_t num_to_write)
{
    int bytes_written{};

    while (bytes_written < num_to_write)
    {
        int num_written = write(fd, buf + bytes_written, num_to_write - bytes_written);
        if (num_written < 0)
            return -1;

        bytes_written += num_written;
    }
    return 0;
}

int32_t one_request(int connfd)
{
    // Protocol: len1, msg1, \n
    // Note: each len will be little endian integer
    std::array<char, LEN_FIELD_SIZE + MAX_MSG_FIELD_SIZE + 1> rbuf{};

    errno = 0;
    // Read LEN_FIELD_SIZE bytes from client and store as msg_len
    auto err = read_full(connfd, rbuf.data(), LEN_FIELD_SIZE);
    if (err)
    {
        if (errno == 0)
            std::cout << "EOF\n";
        else
            std::cout << "read() error\n";
        return err;
    }

    uint32_t msg_len{};
    std::memcpy(&msg_len, rbuf.data(), LEN_FIELD_SIZE);
    if (msg_len > MAX_MSG_FIELD_SIZE)
    {
        std::cout << "Too Long!\n";
        return -1;
    }

    // Read msg_length bytes body  
    err = read_full(connfd, rbuf.data() + LEN_FIELD_SIZE, msg_len);
    if (err)
    {
        std::cout << "read() error\n";
        return err;
    }

    // Perform operation
    rbuf[LEN_FIELD_SIZE + MAX_MSG_FIELD_SIZE] = '\0';
    std::cout << "client says:" << rbuf.data() + LEN_FIELD_SIZE << "\n";

    // Generate a response
    constexpr auto reply{ "world\n" };
    constexpr auto reply_len = (uint32_t)strlen(reply);
    std::array<char, LEN_FIELD_SIZE + reply_len> wbuf{};

    std::memcpy(wbuf.data(), &reply_len, LEN_FIELD_SIZE);
    std::memcpy(wbuf.data() + LEN_FIELD_SIZE, reply, reply_len);

    return write_all(connfd, wbuf.data(), LEN_FIELD_SIZE + reply_len);

}

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
        std::cout << "Failed to create socket. errno: " << errno << "\n";

    int val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);

    if (bind(server_fd, (const sockaddr*)&addr, sizeof(addr)) == -1)
        std::cout << "Failed to bind a sockaddr to server_fd\n";

    listen(server_fd, SOMAXCONN);

    while (true)
    {
        struct sockaddr_in client_addr {};
        socklen_t socklen{ sizeof(client_addr) };
        int connfd = accept(server_fd, (struct sockaddr*)&client_addr, &socklen);
        if (connfd == -1)
        {
            std::cout << "Connection failed with client addr\n";
            continue; // keep alive
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