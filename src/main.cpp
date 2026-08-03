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
#include <fstream>
#include <zlib.h>

std::string findMethod(std::string request)
{
  std::string method;
  int first_space = 0;
  while (first_space < request.size() && request[first_space] != ' ')
    first_space++;
  method = request.substr(0, first_space);
  return method;
}

std::string findPath(std::string request)
{
  int first_space = 0;
  while (first_space < request.size() && request[first_space] != ' ')
    first_space++;

  int next_space = first_space + 1;
  while (next_space < request.size() && request[next_space] != ' ')
    next_space++;

  std::string path(request.substr(first_space + 1, next_space - first_space - 1));

  return path;
}

std::string findAgent(std::string request)
{
  // finding header user agent nd its value , packing nd sending back
  auto start_pos = request.find("User-Agent: ");
  if (start_pos != std::string::npos)
  {
    std::string key = "User-Agent: ";
    start_pos += key.size();

    auto end_pos = request.find("\r\n", start_pos);
    std::string body = request.substr(start_pos, end_pos - start_pos);
    if (body.empty())
    {
      return "";
    }
    return body;
  }
  return "";
}

std::string findBody(std::string request)
{
  auto start_pos = request.find("\r\n\r\n");
  if (start_pos != std::string::npos)
  {
    start_pos += 4; // Move past the "\r\n\r\n"
    std::string body = request.substr(start_pos);
    return body;
  }
  return "";
}

bool acceptsGzip(const std::string &request)
{
  auto start_pos = request.find("Accept-Encoding: ");
  if (start_pos != std::string::npos)
  {
    auto end_pos = request.find("\r\n", start_pos);
    std::string header_value = request.substr(start_pos + 17, end_pos - (start_pos + 17));
    if (header_value.find("gzip") != std::string::npos)
    {
      return true;
    }
  }
  return false;
}

std::string gzipCompress(const std::string &input)
{
  z_stream zs{};
  std::string output;

  if (deflateInit2(
          &zs,
          Z_DEFAULT_COMPRESSION, // around 6 good speed nd compression ratio
          Z_DEFLATED,
          MAX_WBITS + 16, // +16 => gzip instead of zlib
          8,
          Z_DEFAULT_STRATEGY) != Z_OK)
  {
    return "";
  }

  // just finding the index of input string, reinterpret cast means treat bytef nd char* same, const cast removes cont
  zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(input.data()));
  zs.avail_in = input.size();

  // 32 kb standard
  char buffer[32768];

  int ret;

  do
  {
    zs.next_out = reinterpret_cast<Bytef *>(buffer);
    zs.avail_out = sizeof(buffer);

    // deflate return -1 if error, 0 if ok, 1 if stream end
    ret = deflate(&zs, Z_FINISH);

    size_t bytes_written = sizeof(buffer) - zs.avail_out;

    output.append(buffer, bytes_written);

  } while (ret == Z_OK); // continue till ret ==0

  // clear memeory
  deflateEnd(&zs);

  if (ret != Z_STREAM_END)
    return "";

  return output;
}

bool readFile(const std::string &filepath, std::string &body)
{
  std::cout << filepath << '\n';
  std::ifstream file(filepath);
  if (!file.is_open())
    return false;

  body.assign(
      std::istreambuf_iterator<char>(file),
      std::istreambuf_iterator<char>());

  return true;
}

bool writeFile(const std::string &filepath, const std::string &body)
{
  std::cout << "write file called\n";
  std::ofstream file(filepath);
  if (!file.is_open())
    return false;

  file << body;
  return true;
}

std::string buildResponse(const std::string &method, const std::string &path, const std::string &request, const std::string &directory)
{
  std::string response;
  if (path == "/")
  {
    response = "HTTP/1.1 200 OK\r\n\r\n";
  }
  else if (method == "GET" && path.find("/echo/") == 0)
  {
    std::string body = path.substr(6);

    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: text/plain\r\n";

    if (acceptsGzip(request))
    {
      response += "Content-Encoding: gzip\r\n";
      body = gzipCompress(body);
    }
    response +=
        "Content-Length: " +
        std::to_string(body.size()) +
        "\r\n\r\n" +
        body;
  }

  else if (method == "GET" && path == "/user-agent") // rn they just sent /user agent else we would have used find(user-agent)
  {
    std::string body = findAgent(request);

    response = "HTTP/1.1 200 OK\r\n"

               "Content-Type: text/plain\r\n"
               "Content-Length: " +
               std::to_string(body.size()) + "\r\n"
                                             "\r\n" +
               body;
  }

  else if (method == "GET" && path.find("/files/") == 0)
  {
    std::string filename = path.substr(7);

    std::string fullPath = directory + filename;

    std::string body;

    if (readFile(fullPath, body))
    {
      response =
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: application/octet-stream\r\n"
          "Content-Length: " +
          std::to_string(body.size()) +
          "\r\n\r\n" +
          body;
    }
    else
    {
      response = "HTTP/1.1 404 Not Found\r\n\r\n";
    }
  }

  else if (method == "POST" && path.find("/files/") == 0)
  {
    std::string filename = path.substr(7);
    std::string fullPath = directory + filename;
    std::string body = findBody(request);

    if (writeFile(fullPath, body))
    {
      response = "HTTP/1.1 201 Created\r\n\r\n";
    }
    else
    {
      response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
    }
  }

  else
    response = "HTTP/1.1 404 Not Found\r\n\r\n";

  return response;
}

void wthread_handleClient(int client_fd, const std::string &directory)
{
  std::cout << "Worker thread started for client with FD: " << client_fd << "\n";

  // read the request from client
  char buffer[1024];
  ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
  std::cout << "data received from client: " << client_fd << ": " << bytes_read << "\n";
  std::string request(buffer, bytes_read); // request is the full request from the client so it contains everything from get url to http 1.1 nd headers

  // finding url path first word so its easy to check for the request url starting
  std::string path = findPath(request);
  std::string method = findMethod(request);

  // a body for the response to send back to the client
  std::string response = buildResponse(method, path, request, directory);

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
