#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h> // inet_pton()
#include <assert.h>
#include <errno.h>
#include <iostream>
#include <netinet/ip.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h> // socket()
#include <unistd.h>
#include <vector>

class TcpTransport
{
public:
    TcpTransport()
    {
        client_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if ( client_fd_ < 0 )
        {
            // Handle error
        }
    }

    void connect(std::string const& address, int const port)
    {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if ( inet_pton(AF_INET, address.c_str(), &addr.sin_addr) <= 0 )
        {
            // Handle error
            return;
        }

        if ( ::connect(client_fd_, (sockaddr const*)&addr, sizeof(addr)) < 0 )
        {
            // Handle error
            return;
        }
    }

    void send(std::vector<uint8_t> const& data, int const flags = 0)
    {
        ::send(client_fd_, data.data(), data.size(), flags);
    }

    std::vector<uint8_t> receive(int buffer_size, int const flags = 0)
    {
        std::vector<uint8_t> buf(buffer_size);

        ssize_t num_received = ::recv(client_fd_, buf.data(), buf.size(), flags);
        if ( num_received < 0 )
        {
            // Handle error
        }
        return buffer;
    }

    ~TcpTransport()
    {
        ::close(client_fd_);
    }

private:
    int client_fd_{ -1 };
};

class RedisSerializer
{
public:
    std::vector<uint8_t> serialize(char** message)
    {

        std::vector<uint8_t> out{};

        return out;
    }
};

class RedisDeserializer
{
public:
    std::string deserialize(std::vector<uint8_t> const& message)
    {
    }
};

template <typename Transport, typename Serializer, typename Deserializer>
class SocketClient
{
public:
    SocketClient(Transport& transport, Serializer& serializer, Deserializer& deserializer)
        : transport_(transport), serializer_(serializer), deserializer_(deserializer)
    {
    }

    void connect(std::string const& address, int const port)
    {
        transport_.connect(address, port);
    }

    // void send_message(std::string const& message)
    // {
    // auto data = serializer_.serialize(message);
    // transport_.send(data);
    // }

    void send_message(char** message)
    {
        auto data = serializer_.serialize(message);
        transport_.send(data);
    }

    void receive_message()
    {
        auto data = transport_.receive();
        return serializer.deserialize(data);
    }

private:
    Transport& transport_;
    Serializer& serializer_;
    Deserializer& deserializer_;
};

class CLIHandler
{
public:
    CLIHandler(SocketClient<TcpTransport, RedisSerializer, RedisDeserializer>& client) : client_(client)
    {
    }

    void process_args(int argc, char** argv)
    {
        // Smallest command looks like ./client 127.0.0.1 1234 SOME_CMD
        if ( argc < 4 )
        {
            // Handle error
            return;
        }

        std::string server = argv[1];
        int port = std::stoi(argv[2]);

        client_.connect(server, port);
        client_.send_message(argv[3]);
    }

private:
    SocketClient<TcpTransport, RedisSerializer, RedisDeserializer>& client_;
};

#endif