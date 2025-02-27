#include "client.h"

int main(int argc, char** argv)
{
    TcpTransport transport;
    RedisSerializer serializer;
    RedisDeserializer deserializer;

    SocketClient<TcpTransport, RedisSerializer, RedisDeserializer> client(transport, serializer, deserializer);
    CLIHandler cli(client);
    cli.process_args(argc, argv);

    return 0;
}
