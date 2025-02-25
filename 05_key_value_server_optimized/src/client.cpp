#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

static constexpr int PORT{ 1234 };
static constexpr uint8_t LEN_FIELD_SIZE{ 4 };
static constexpr size_t MAX_MSG_FIELD_SIZE{ 32 << 20 };

bool read_full(int fd, uint8_t* buf, size_t num_to_read)
{
    int bytes_read{};

    while ( bytes_read < num_to_read )
    {
        int num_read = read(fd, buf + bytes_read, num_to_read - bytes_read);
        if ( num_read < 0 )
            return false;

        bytes_read += num_read;
    }
    return true;
}

bool write_all(int fd, uint8_t const* buf, size_t num_to_write)
{
    int bytes_written{};

    while ( bytes_written < num_to_write )
    {
        int num_written = write(fd, buf + bytes_written, num_to_write - bytes_written);
        if ( num_written < 0 )
            return false;

        bytes_written += num_written;
    }
    return true;
}

bool send_request(int client_sock, uint8_t const* request, size_t data_len)
{
    if ( data_len > MAX_MSG_FIELD_SIZE )
        return false;
    static bool send_once{ true };
    std::vector<uint8_t> wbuf{};

    if ( send_once )
    {
        wbuf.insert(wbuf.end(), 5);
        send_once = false;
    }

    auto p = reinterpret_cast<uint8_t const*>(&data_len);
    wbuf.insert(wbuf.end(), p, p + LEN_FIELD_SIZE);
    wbuf.insert(wbuf.end(), request, request + data_len);

    if ( !write_all(client_sock, wbuf.data(), wbuf.size()) )
    {
        std::cout << "Failed to write request LEN, to server\n";
        return false;
    }

    return true;
}

bool read_response(int client_sock)
{
    std::vector<uint8_t> rbuf(LEN_FIELD_SIZE);

    if ( !read_full(client_sock, rbuf.data(), LEN_FIELD_SIZE) )
    {
        std::cout << "Failed to read response LEN, from server\n";
        return false;
    }

    uint32_t data_len;
    std::memcpy(&data_len, rbuf.data(), LEN_FIELD_SIZE);
    if ( data_len > MAX_MSG_FIELD_SIZE )
        return false;

    rbuf.resize(LEN_FIELD_SIZE + data_len);

    if ( !read_full(client_sock, rbuf.data() + LEN_FIELD_SIZE, data_len) )
    {
        std::cout << "Failed to read response DATA, from server\n";
        return false;
    }

    std::cout << "Len: " << data_len << ", response: "
              << std::string_view(reinterpret_cast<char const*>(rbuf.data() + LEN_FIELD_SIZE),
                                  std::min<size_t>(data_len, 100))
              << std::endl;
    return true;
}

int setup_client_connection()
{
    // Create socket
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if ( client_sock < 0 )
    {
        std::cout << "Failed to create socket" << std::endl;
    }

    int opt = 1;
    setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if ( connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 )
    {
        std::cout << "Connection failed!" << std::endl;
        close(client_sock);
    }
    std::cout << "Connected to the server." << std::endl;

    return client_sock;
}

int main()
{

    int client_sock = setup_client_connection();
    if ( client_sock < 0 )
        return 1;

    std::vector<std::string> query_list = {
        "hello1",
        "hello2",
        "hello3",
        // a large message requires multiple event loop iterations
        std::string(1000, 'z'),
        "hello5",
    };

    for ( auto const& query : query_list )
    {
        if ( !send_request(client_sock, (uint8_t*)query.data(), query.size()) )
            std::cout << "Failed to send request: " << query << std::endl;
    }

    for ( int i = 0; i < query_list.size(); ++i )
    {
        if ( !read_response(client_sock) )
            std::cout << "Failed to read response" << std::endl;
    }

    // Close the socket
    close(client_sock);
    std::cout << "Connection closed." << std::endl;

    return 0;
}
