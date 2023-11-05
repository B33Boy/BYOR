#include <iostream>
#include <string.h> // strlen
#include <unistd.h> // read/write
#include <sys/socket.h> // Socket functions
#include <netinet/ip.h> // sockaddr_in struct

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

    if (client_fd < 0)
    {
        std::cout << "Failed to create socket. errno: " << errno << "\n"; 
    }
    
    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = ntohs(1234);
    // INADDR_LOOPBACK means only clients on the same machine can connect to it
    server_addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); 
    
    if (connect(client_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
    {
        std::cout << "Failed to connect to server\n";
    }

    char msg[]{"hello"};
    write(client_fd, msg, strlen(msg));

    // n is the number of bytes read, can be within 0 (reached end of fd) and size of rbuf
    char rbuf[64]{};
    ssize_t n = read(client_fd, rbuf, sizeof(rbuf));
    if (n < 0)
    {
        std::cout << "Cannot read from fd\n";
    }

    std::cout << "server says: " << rbuf << "\n";

    return 0;
}