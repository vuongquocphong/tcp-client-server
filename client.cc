#include "client.h"

Client::Client(const std::string &server_ip, int server_port)
    : server_ip(server_ip), server_port(server_port), socket_fd(-1), running(false)
{
}

Client::~Client()
{
    stop();
}

void Client::connect_to_server()
{
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr
    {
    };
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Connected to the server at " << server_ip << ":" << server_port << "\n";
}

void Client::receive_messages()
{
    char buffer[BUFFER_SIZE];
    while (running)
    {
        int bytes_received = recv(socket_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received > 0)
        {
            buffer[bytes_received] = '\0';
            std::cout << "Server: " << buffer << "\n";
        }
        else if (bytes_received == 0)
        {
            std::cout << "Server has shut down. Closing client.\n";
            running = false;
            break;
        }
        else
        {
            perror("recv");
            running = false;
            break;
        }
    }
}

void Client::send_message(const std::string &message)
{
    if (send(socket_fd, message.c_str(), message.size(), 0) == -1)
    {
        perror("send");
    }
}

void Client::start()
{
    connect_to_server();
    running = true;

    receiver_thread = std::thread(&Client::receive_messages, this);

    while (running)
    {
        std::string message;
        std::cout << "You: ";
        std::getline(std::cin, message);

        if (message == "exit")
        {
            std::cout << "Disconnecting...\n";
            running = false;
            break;
        }

        send_message(message);
    }

    stop();
}

void Client::stop()
{
    if (running)
    {
        running = false;
    }
    if (socket_fd != -1)
    {
        close(socket_fd);
    }
    if (receiver_thread.joinable())
    {
        receiver_thread.join();
    }
    std::cout << "Client shut down.\n";
}
