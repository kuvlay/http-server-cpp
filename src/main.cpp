#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include "http/http.hpp"

int main(int argc, char **argv)
{

  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::cout << "Logs from program will appear here!\n";

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
  {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }

  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
  {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
  {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0)
  {
    std::cerr << "listen failed\n";
    return 1;
  }

  // my 2nd socket for client connection
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";

  std::string directory = "";
  if (argc == 3 && std::string(argv[1]) == "--directory")
    directory = argv[2];
  std::cout << "Directory = " << directory << '\n';

  // check for all clients/server doesnt close connection byself
  while (true)
  {
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
    std::cout << "Accepted connected\n";

    std::cout << "Worker Thread created for client with FD: " << client_fd << "\n";

    std::thread worker(wthread_handleClient, client_fd, directory);

    worker.detach();
  }

  close(server_fd);

  return 0;
}
