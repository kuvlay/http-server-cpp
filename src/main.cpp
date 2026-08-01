#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";
  
  
  
  while(true){
    int client_fd=accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    std::cout << "Client connected\n";
    std::cout << "I AM HERE\n";
  
    char buffer[1024];
  
    ssize_t bytes_read = recv(client_fd,buffer,sizeof(buffer),0);
    
    std::cout << "bytes_read = "
            << bytes_read
            << '\n';
  
    int first_space =0;
    while(first_space < bytes_read && buffer[first_space]!=' ') first_space++;
  
    int next_space =first_space+1;
    while(next_space < bytes_read && buffer[next_space]!=' ') next_space++;
    
    std::string path(buffer+first_space+1, next_space-first_space-1);
    std::cout << path << '\n';
  
    std::string response;
  
    if(path=="/") response = "HTTP/1.1 200 OK\r\n\r\n";
    else if(path.rfind("/echo/", 0) == 0){
      std::string body = path.substr(6);
  
      response = 
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: " + std::to_string(body.size())+
                            "\r\n\r\n" +
                            body;
      
      //send(client_fd,response,strlen(response),0);
  
    }
  
    else response = "HTTP/1.1 404 Not Found\r\n\r\n";
    
    send(client_fd, response.c_str(), response.size(), 0);
  
    close(client_fd);
  }

  close(server_fd);

  return 0;
}
