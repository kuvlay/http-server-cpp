#include "http.hpp"
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include "file/file.hpp"
#include "gzip/gzip.hpp"

HttpRequest parseRequest(const std::string &request)
{
    HttpRequest parsedRequest;

    std::size_t requestLineEnd = request.find("\r\n");
    std::string requestLine = request.substr(0, requestLineEnd);

    std::istringstream requestLineStream(requestLine);
    requestLineStream >> parsedRequest.method >> parsedRequest.path >> parsedRequest.version;

    std::size_t headersStart = requestLineEnd == std::string::npos ? request.size() : requestLineEnd + 2;
    std::size_t bodyStart = request.find("\r\n\r\n");
    std::size_t headersEnd = bodyStart == std::string::npos ? request.size() : bodyStart;

    std::size_t currentPosition = headersStart;
    while (currentPosition < headersEnd)
    {
        std::size_t nextLineEnd = request.find("\r\n", currentPosition);
        if (nextLineEnd == std::string::npos || nextLineEnd > headersEnd)
            nextLineEnd = headersEnd;

        std::string headerLine = request.substr(currentPosition, nextLineEnd - currentPosition);
        if (headerLine.empty())
            break;

        std::size_t colonPosition = headerLine.find(": ");
        if (colonPosition != std::string::npos)
        {
            std::string headerKey = headerLine.substr(0, colonPosition);
            std::string headerValue = headerLine.substr(colonPosition + 2);
            parsedRequest.headers[headerKey] = headerValue;
        }

        currentPosition = nextLineEnd + 2;
    }

    if (bodyStart != std::string::npos)
    {
        parsedRequest.body = request.substr(bodyStart + 4);
    }

    return parsedRequest;
}

void addHeader(std::string &response, const std::string &key, const std::string &value)
{
    response += key + ": " + value + "\r\n";
}

std::string buildResponse(const HttpRequest &request, const std::string &directory, bool shouldClose)
{
    std::string response;
    const auto connectionHeader = request.headers.find("Connection");
    const auto acceptEncodingHeader = request.headers.find("Accept-Encoding");
    const auto userAgentHeader = request.headers.find("User-Agent");

    if (request.path == "/")
    {
        response = "HTTP/1.1 200 OK\r\n";

        if (shouldClose)
            addHeader(response, "Connection", "close");

        response += "\r\n";
    }

    else if (request.method == "GET" && request.path.find("/echo/") == 0)
    {
        std::string body = request.path.substr(6);

        response = "HTTP/1.1 200 OK\r\n";

        addHeader(response, "Content-Type", "text/plain");

        if (shouldClose)
            addHeader(response, "Connection", "close");

        if (acceptEncodingHeader != request.headers.end() && acceptEncodingHeader->second.find("gzip") != std::string::npos)
        {
            addHeader(response, "Content-Encoding", "gzip");
            body = gzipCompress(body);
        }

        addHeader(response, "Content-Length", std::to_string(body.size()));

        response += "\r\n";
        response += body;
    }

    else if (request.method == "GET" && request.path == "/user-agent")
    {
        std::string body;
        if (userAgentHeader != request.headers.end())
            body = userAgentHeader->second;

        response = "HTTP/1.1 200 OK\r\n";

        addHeader(response, "Content-Type", "text/plain");

        if (shouldClose)
            addHeader(response, "Connection", "close");

        addHeader(response, "Content-Length", std::to_string(body.size()));

        response += "\r\n";
        response += body;
    }

    else if (request.method == "GET" && request.path.find("/files/") == 0)
    {
        std::string filename = request.path.substr(7);
        std::string fullPath = directory + filename;

        std::string body;

        if (readFile(fullPath, body))
        {
            response = "HTTP/1.1 200 OK\r\n";

            addHeader(response, "Content-Type", "application/octet-stream");

            if (shouldClose)
                addHeader(response, "Connection", "close");

            addHeader(response, "Content-Length", std::to_string(body.size()));

            response += "\r\n";
            response += body;
        }
        else
        {
            response = "HTTP/1.1 404 Not Found\r\n";

            if (shouldClose)
                addHeader(response, "Connection", "close");

            response += "\r\n";
        }
    }

    else if (request.method == "POST" && request.path.find("/files/") == 0)
    {
        std::string filename = request.path.substr(7);
        std::string fullPath = directory + filename;
        std::string body = request.body;

        if (writeFile(fullPath, body))
        {
            response = "HTTP/1.1 201 Created\r\n";

            if (shouldClose)
                addHeader(response, "Connection", "close");

            response += "\r\n";
        }
        else
        {
            response = "HTTP/1.1 500 Internal Server Error\r\n";

            if (shouldClose)
                addHeader(response, "Connection", "close");

            response += "\r\n";
        }
    }

    else
    {
        response = "HTTP/1.1 404 Not Found\r\n";

        if (shouldClose)
            addHeader(response, "Connection", "close");

        response += "\r\n";
    }

    return response;
}

void wthread_handleClient(int client_fd, const std::string &directory)
{
    std::cout << "Worker thread started for client with FD: " << client_fd << "\n";

    while (true)
    {
        char buffer[1024];
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
        std::cout << "data received from client: " << client_fd << ": " << bytes_read << "\n";

        if (bytes_read <= 0)
        {
            std::cout << "Client with FD: " << client_fd << " disconnected or error occurred\n";
            break;
        }

        std::string request(buffer, bytes_read);
        HttpRequest parsedRequest = parseRequest(request);
        bool closeConnection = parsedRequest.headers.find("Connection") != parsedRequest.headers.end() && parsedRequest.headers.find("Connection")->second.find("close") != std::string::npos;

        std::string response = buildResponse(parsedRequest, directory, closeConnection);

        send(client_fd, response.c_str(), response.size(), 0);
        if (closeConnection)
        {
            std::cout << "Closing connection with client FD: " << client_fd << "\n";
            break;
        }
    }

    close(client_fd);

    std::cout << "Worker thread finished for client with FD: " << client_fd << "\n";
}