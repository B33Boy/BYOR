#include "server.h"

int main()
{
    uint16_t  port{ 1234 };
    Server server(port);
    server.start();

    return 0;
}