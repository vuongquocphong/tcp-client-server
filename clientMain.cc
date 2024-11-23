#include "client.h"

int main()
{
    const std::string SERVER_IP = "127.0.0.1";
    const int SERVER_PORT = 8080;

    Client client(SERVER_IP, SERVER_PORT);
    client.start();

    return 0;
}
