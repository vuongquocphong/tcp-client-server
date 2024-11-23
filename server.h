#pragma once
#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

const int MAX_EVENTS = 10;
const int BUFFER_SIZE = 1024;

class Server
{
  public:
    Server(int port);
    ~Server();
    void start();
    void stop();

  private:
    int server_fd;
    int port;
    std::unordered_map<int, std::string> clients;
    std::mutex clients_mutex;
    std::atomic<bool> running;
    int epoll_fd;

    void make_socket_non_blocking(int socket_fd);
    void command_handler();
    void client_handler();
    void reply_to_client(int client_fd, const std::string &message);
};