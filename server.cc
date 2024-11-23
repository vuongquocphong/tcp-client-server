#include "server.h"

Server::Server(int port) : port(port), running(false), epoll_fd(-1)
{
}

Server::~Server()
{
    stop();
}

void Server::make_socket_non_blocking(int socket_fd)
{
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
}

void Server::reply_to_client(int client_fd, const std::string &message)
{
    if (send(client_fd, message.c_str(), message.size(), 0) == -1)
    {
        perror("send");
    }
    else
    {
        std::cout << "Replied to client FD " << client_fd << ": " << message << "\n";
    }
}

void Server::command_handler()
{
    while (running)
    {
        std::string command;
        std::getline(std::cin, command);

        if (command == "exit")
        {
            running = false;
            break;
        }
        else if (command.rfind("broadcast ", 0) == 0)
        {
            std::string message = command.substr(10);
            std::lock_guard<std::mutex> lock(clients_mutex);
            for (const auto &[client_fd, _] : clients)
            {
                send(client_fd, message.c_str(), message.size(), 0);
            }
        }
        else if (command == "list")
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            std::cout << "Connected clients:\n";
            for (const auto &[client_fd, address] : clients)
            {
                std::cout << "Client FD: " << client_fd << ", Address: " << address << "\n";
            }
        }
        else
        {
            std::cout << "Unknown command: " << command << "\n";
        }
    }
}

void Server::client_handler()
{
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1)
    {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[MAX_EVENTS];
    char buffer[BUFFER_SIZE];

    while (running)
    {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
        if (n == -1)
        {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i)
        {
            if (events[i].data.fd == server_fd)
            {
                // New connection
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                if (client_fd == -1)
                {
                    perror("accept");
                    continue;
                }

                make_socket_non_blocking(client_fd);
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
                {
                    perror("epoll_ctl");
                    close(client_fd);
                    continue;
                }

                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    clients[client_fd] = inet_ntoa(client_addr.sin_addr);
                }
                std::cout << "New client connected: FD " << client_fd << "\n";
            }
            else
            {
                // Client message
                int client_fd = events[i].data.fd;
                int bytes_read = recv(client_fd, buffer, BUFFER_SIZE, 0);
                if (bytes_read <= 0)
                {
                    // Client disconnected
                    {
                        std::lock_guard<std::mutex> lock(clients_mutex);
                        clients.erase(client_fd);
                    }
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
                    close(client_fd);
                    std::cout << "Client disconnected: FD " << client_fd << "\n";
                }
                else
                {
                    buffer[bytes_read] = '\0';
                    std::cout << "Message from client FD " << client_fd << ": " << buffer << "\n";

                    // Reply to the client
                    std::string reply = "Server received: ";
                    reply += buffer;
                    reply_to_client(client_fd, reply);
                }
            }
        }
    }

    close(epoll_fd);
}

void Server::start()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr
    {
    };
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, SOMAXCONN) == -1)
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    make_socket_non_blocking(server_fd);
    running = true;

    std::cout << "Server started on port " << port << "\n";

    std::thread command_thread(&Server::command_handler, this);
    std::thread client_thread(&Server::client_handler, this);

    command_thread.join();
    client_thread.join();
}

void Server::stop()
{
    if (running)
    {
        running = false;
        close(server_fd);
        std::cout << "Server shut down.\n";
    }
}