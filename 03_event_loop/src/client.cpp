#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>

int main() {
    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed!" << std::endl;
        return 1;
    }

    // Allow reusing the address (important for avoiding TIME_WAIT issues)
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Define server address
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1234); // Server port
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP address (localhost)

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed!" << std::endl;
        close(sock);
        return 1;
    }
    std::cout << "Connected to the server." << std::endl;

    // Data to send to the server
    const char* message = "123456789";

    // Send data to the server
    ssize_t bytes_sent = send(sock, message, strlen(message), 0);
    if (bytes_sent < 0) {
        std::cerr << "Failed to send data!" << std::endl;
        close(sock);
        return 1;
    }
    std::cout << "Sent " << bytes_sent << " bytes: " << message << std::endl;

    // Receive the response from the server
    char buffer[1024] = { 0 };
    ssize_t bytes_received = recv(sock, buffer, sizeof(buffer), 0);
    if (bytes_received < 0) {
        std::cerr << "Failed to receive data!" << std::endl;
        close(sock);
        return 1;
    }
    std::cout << "Received " << bytes_received << " bytes: " << buffer << std::endl;

    // Close the socket
    close(sock);
    std::cout << "Connection closed." << std::endl;

    return 0;
}
