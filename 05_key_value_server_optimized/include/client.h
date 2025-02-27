#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h> // sockaddr_in, inet_pton, htons
#include <cstdint>     // uint8_t, uint32_t
#include <cstring>     // std::memcpy
#include <iostream>    // std::cout, std::cerr
#include <sstream>
#include <string>       // std::string, std::stoi
#include <sys/socket.h> // socket, connect, send, recv
#include <unistd.h>     // close
#include <vector>       // std::vector

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

    std::vector<uint8_t> receive(size_t buffer_size = 32 << 20, int const flags = 0)
    {
        std::vector<uint8_t> buf(buffer_size);

        ssize_t num_received = ::recv(client_fd_, buf.data(), buf.size(), flags);
        if ( num_received < 0 )
        {
            // Handle error
        }
        return buf;
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
    std::vector<uint8_t> serialize(std::vector<std::string>& message)
    {
        // +---------+------+------+------+------+------+-----+------+------+
        // |  nbytes | nstr | len0 | cmd0 | len1 | cmd1 | ... | lenn | cmdn |
        // +---------+------+------+------+------+------+-----+------+------+
        // Note: For [nstr, len0, cmd0], nstr would be 1

        uint32_t total_len = 2 * LEN_FIELD_SIZE + LEN_FIELD_SIZE * message.size();
        for ( auto const& s : message )
            total_len += s.length();

        std::vector<uint8_t> wbuf(total_len);

        uint8_t* p = wbuf.data();

        // Populate nbytes
        total_len -= LEN_FIELD_SIZE; // Compute size of all bytes excluding nbytes
        memcpy(p, &total_len, LEN_FIELD_SIZE);
        p += LEN_FIELD_SIZE;

        // Populate nstr
        auto nstr = static_cast<uint32_t>(message.size());
        memcpy(p, &nstr, LEN_FIELD_SIZE);
        p += LEN_FIELD_SIZE;

        // Populate len0, cmd0, ...
        for ( auto const& s : message )
        {
            uint32_t len = static_cast<uint32_t>(s.length());
            memcpy(p, &len, LEN_FIELD_SIZE);
            p += LEN_FIELD_SIZE;

            memcpy(p, s.data(), len);
            p += len;
        }

        return wbuf;
    }

private:
    int LEN_FIELD_SIZE{ 4 };
};

class RedisDeserializer
{
public:
    std::string deserialize(std::vector<uint8_t> const& message)
    {
        // Handle <4 bytes

        // Parse data length
        uint32_t data_len{};
        memcpy(&data_len, message.data(), sizeof(data_len));

        auto it = message.begin() + sizeof(data_len);

        // Handle if data_len exceeds buffer size

        return make_response(message, it);
    }

private:
    std::string make_response(std::vector<uint8_t> const& data, std::vector<uint8_t>::const_iterator& it)
    {
        std::ostringstream resp;

        resp << "Response Code: " << static_cast<int>(*it) << "\n";
        it++;

        resp << "Response: ";
        while ( it != data.end() )
        {
            resp << static_cast<char>(*it);
            ++it;
        }

        return resp.str();
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

    void send_message(std::vector<std::string>& message)
    {
        auto data = serializer_.serialize(message);
        transport_.send(data);
    }

    std::string receive_message()
    {
        auto data = transport_.receive();
        return deserializer_.deserialize(data);
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

        std::string const server = argv[1];
        int const port = std::stoi(argv[2]);
        client_.connect(server, port);

        int const cmd_size{ argc - 3 };
        std::vector<std::string> cmds(cmd_size);
        for ( int i = 0; i < cmd_size; i++ )
            cmds[i] = argv[i + 3];

        client_.send_message(cmds);
        std::cout << "Client returned: " << client_.receive_message() << "\n";
    }

private:
    SocketClient<TcpTransport, RedisSerializer, RedisDeserializer>& client_;
};

#endif