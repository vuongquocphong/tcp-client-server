#pragma once
#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <fcntl.h> // For non-blocking socket

const int BUFFER_SIZE = 1024;

class Client
{
public:
    Client(const std::string &server_ip, int server_port);
    ~Client();
    void start();

private:
    std::string server_ip;
    int server_port;
    int socket_fd;
    std::atomic<bool> running;      // Controls the main loop
    std::atomic<bool> connected;   // Tracks connection status
    std::thread communication_thread;

    void connect_to_server();
    void reconnect_to_server();
    void communication_loop();
    void send_message(const std::string &message);
    void close_connection();
    void set_socket_nonblocking(int fd);
};
