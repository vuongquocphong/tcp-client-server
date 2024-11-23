#pragma once
#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

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
    std::atomic<bool> running;
    std::thread receiver_thread;

    void connect_to_server();
    void receive_messages();
    void send_message(const std::string &message);
    void stop();
};
