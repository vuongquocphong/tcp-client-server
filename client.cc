#include "client.h"

Client::Client(const std::string &server_ip, int server_port)
    : server_ip(server_ip), server_port(server_port), socket_fd(-1), running(false), connected(false)
{
}

Client::~Client() = default;

void Client::connect_to_server()
{
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        perror("socket");
        return;
    }

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(socket_fd);
        socket_fd = -1;
        return;
    }

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0)
    {
        std::cout << "Connected to the server at " << server_ip << ":" << server_port << "\n";
        connected = true;
        set_socket_nonblocking(socket_fd); // Optional: Make socket non-blocking
    }
    else
    {
        perror("connect");
        close(socket_fd);
        socket_fd = -1;
    }
}

void Client::reconnect_to_server()
{
    close_connection();
    std::cout << "Attempting to reconnect...\n";
    connect_to_server();
    if (connected && !communication_thread.joinable())
    {
        communication_thread = std::thread(&Client::communication_loop, this);
    }
}

void Client::set_socket_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl (F_GETFL)");
        return;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl (F_SETFL)");
    }
}

void Client::communication_loop()
{
    char buffer[BUFFER_SIZE];

    while (connected)
    {
        // Receive message from server
        int bytes_received = recv(socket_fd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received > 0)
        {
            buffer[bytes_received] = '\0';
            std::cout << "Server: " << buffer << "\n";
        }
        else if (bytes_received == 0)
        {
            std::cout << "Server has shut down. Disconnecting...\n";
            connected = false;
            break;
        }
        else if (errno != EWOULDBLOCK && errno != EAGAIN) // Non-blocking mode check
        {
            perror("recv");
            connected = false;
            break;
        }

        // Avoid busy-waiting in non-blocking mode
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Client::send_message(const std::string &message)
{
    // Attempt to reconnect if not connected
    if (!connected)
    {
        std::cout << "Not connected to server. Attempting to reconnect...\n";
        reconnect_to_server();
        if (!connected)
        {
            std::cout << "Failed to send message: Cannot connect to server.\n";
            return;
        }
    }

    // Send message
    if (send(socket_fd, message.c_str(), message.size(), 0) == -1)
    {
        perror("send");
    }
}

void Client::close_connection()
{
    connected = false;
    if (socket_fd != -1)
    {
        close(socket_fd);
        socket_fd = -1;
    }
    if (communication_thread.joinable())
    {
        communication_thread.join();
    }
    std::cout << "Disconnected from server.\n";
    std::cout << "Client exited successfully.\n";
}

void Client::start()
{
    connect_to_server();

    if (connected)
    {
        communication_thread = std::thread(&Client::communication_loop, this);
    }

    running = true;

    while (running)
    {
        std::string input;
        std::cout << "You: ";
        std::getline(std::cin, input);

        if (input == "exit")
        {
            std::cout << "Exiting client...\n";
            running = false;
            break;
        }

        // Check for "server " prefix
        if (input.rfind("server ", 0) == 0)
        {
            send_message(input.substr(7)); // Send only the message part
        }
        else
        {
            std::cout << "Invalid command. Messages to server must start with 'server '.\n";
        }
    }

    close_connection();
}
