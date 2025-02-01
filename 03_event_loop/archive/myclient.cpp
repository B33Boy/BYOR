#include <iostream>

#include <cstring> // strlen
#include <unistd.h> // read/write
#include <sys/socket.h> // Socket functions
#include <netinet/ip.h> // sockaddr_in struct

constexpr size_t LEN_FIELD_SIZE{ 4 };
constexpr size_t MAX_MSG_FIELD_SIZE{ 16 };

// ============================= Socket Handling =============================
int create_client_socket()
{
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1)
        std::cout << "Failed to create socket. errno: " << errno << "\n";
    return client_fd;
}

void connect_to_server(int client_fd)
{
    struct sockaddr_in server_addr {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = ntohs(1234);
    server_addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);

    if (connect(client_fd, (const struct sockaddr*)&server_addr, sizeof(server_addr)))
        std::cout << "Failed to connect to server\n";
}


// ============================= Protocol Parsing =============================

int32_t read_full(int fd, char* buf, size_t num_to_read)
{
    int bytes_read{};

    while (bytes_read < num_to_read)
    {
        int num_read = read(fd, buf + bytes_read, num_to_read - bytes_read);
        if (num_read < 0)
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

int32_t query(int fd, const char* text)
{
    auto const query_len = std::strlen(text);
    if (query_len > MAX_MSG_FIELD_SIZE)
        return -1;

    // Populate the write buffer
    char wbuf[LEN_FIELD_SIZE + MAX_MSG_FIELD_SIZE];
    std::memcpy(wbuf, &query_len, LEN_FIELD_SIZE);
    std::memcpy(&wbuf[LEN_FIELD_SIZE], text, query_len);

    if (write_all(fd, wbuf, LEN_FIELD_SIZE + query_len))
    {
        std::cout << "Failed to write to server\n";
        return -1;
    }

    char rbuf[LEN_FIELD_SIZE + MAX_MSG_FIELD_SIZE + 1];
    errno = 0;

    // Read Length
    int32_t err = read_full(fd, rbuf, LEN_FIELD_SIZE);
    if (err)
    {
        if (errno == 0)
            std::cout << "EOF\n";
        else
            std::cout << "read() error\n";
        return err;
    }

    uint32_t msg_len{};
    std::memcpy(&msg_len, rbuf, LEN_FIELD_SIZE);
    if (msg_len > MAX_MSG_FIELD_SIZE)
    {
        std::cout << "too long!\n";
        return -1;
    }

    // Read Contents
    err = read_full(fd, &rbuf[LEN_FIELD_SIZE], msg_len);
    if (err)
    {
        std::cout << "read error\n";
        return -1;
    }

    rbuf[LEN_FIELD_SIZE + msg_len] = '\0';

    // Print output
    std::cout << "server says " << &rbuf[LEN_FIELD_SIZE];

    return 0;
}


int main()
{
    int client_fd = create_client_socket();
    if (client_fd == -1) return 1;

    connect_to_server(client_fd);

    auto err = query(client_fd, "short");
    if (err) { close(client_fd); return 1; }

    err = query(client_fd, "largequery");
    if (err) { close(client_fd); return 1; }

    err = query(client_fd, "extralargequery");
    if (err) { close(client_fd); return 1; }

    close(client_fd);

    return 0;
}