#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>

void wthread_handleClient(int client_fd)
{
  std::cout << "Worker thread started for client with FD: " << client_fd << "\n";

  // read the request from client
  char buffer[1024];
  ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
  std::cout << "data received from client: " << client_fd << ": " << bytes_read << "\n";
  std::string request(buffer, bytes_read); // request is the full request from the client so it contains everything from get url to http 1.1 nd headers

  // finding url path first word so its easy to check for the request url starting
  int first_space = 0;
  while (first_space < bytes_read && buffer[first_space] != ' ')
    first_space++;

  int next_space = first_space + 1;
  while (next_space < bytes_read && buffer[next_space] != ' ')
    next_space++;

  std::string path(buffer + first_space + 1, next_space - first_space - 1);
  std::cout << path << '\n';

  // a body for the response to send back to the client
  std::string response;

  if (path == "/")
  {
    response = "HTTP/1.1 200 OK\r\n\r\n";
  }
  else if (path.find("/echo/") == 0)
  {
    std::string body = path.substr(6);

    response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: " +
        std::to_string(body.size()) +
        "\r\n\r\n" +
        body;
  }

  else if (path == "/user-agent") // rn they just sent /user agent else we would have used find(user-agent)
  {
    // finding header user agent nd its value , packing nd sending back
    ssize_t start_pos = request.find("User-Agent: ");
    if (start_pos != std::string::npos)
    {
      std::string key = "User-Agent: ";
      start_pos += key.size();

      ssize_t end_pos = request.find("\r\n", start_pos);
      std::string body = request.substr(start_pos, end_pos - start_pos);

      response = "HTTP/1.1 200 OK\r\n"

                 "Content-Type: text/plain\r\n"
                 "Content-Length: " +
                 std::to_string(body.size()) + "\r\n"
                                               "\r\n" +
                 body;
    }
  }

  else
    response = "HTTP/1.1 404 Not Found\r\n\r\n";

  send(client_fd, response.c_str(), response.size(), 0);

  close(client_fd);

  std::cout << "Worker thread finished for client with FD: " << client_fd << "\n";
}

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

  // check for all clients/server doesnt close connection byself
  while (true)
  {
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
    std::cout << "Accepted connected\n";

    std::cout << "Worker Thread created for client with FD: " << client_fd << "\n";

    std::thread worker(wthread_handleClient, client_fd);

    worker.detach();
  }

  close(server_fd);

  return 0;
}
