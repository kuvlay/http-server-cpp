#include "http.hpp"
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include "socket/websocket.hpp"
#include "http_types.hpp"
#include "routes.hpp"

std::string serializeResponse(const HttpResponse &response)
{
    std::string raw;

    raw.reserve(
        response.version.size() +
        response.statusMessage.size() +
        response.body.size() +
        response.headers.size() * 32);

    raw += response.version;
    raw += ' ';
    raw += std::to_string(response.statusCode);
    raw += ' ';
    raw += response.statusMessage;
    raw += "\r\n";

    for (const auto &[key, value] : response.headers)
    {
        raw += key;
        raw += ": ";
        raw += value;
        raw += "\r\n";
    }

    raw += "\r\n";

    raw += response.body;

    return raw;
}

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

HttpResponse buildResponse(const HttpRequest &request, const std::string &directory, bool shouldClose)
{
    if (request.path == "/")
        return handleRoot(shouldClose);

    if (request.method == "GET" && request.path.rfind("/echo/", 0) == 0)
        return handleEcho(request, shouldClose);

    if (request.method == "GET" && request.path == "/user-agent")
        return handleUserAgent(request, shouldClose);

    if (request.method == "GET" && request.path.rfind("/files/", 0) == 0)
        return handleGetFile(request, directory, shouldClose);

    if (request.method == "POST" && request.path.rfind("/files/", 0) == 0)
        return handlePostFile(request, directory, shouldClose);

    return handleNotFound(shouldClose);
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

        std::string request(buffer, bytes_read);           // convert into string
        HttpRequest parsedRequest = parseRequest(request); // parse nd store in httprequest struct
        
        // check for websocket upgrade request and handle it
        if (isWebSocketUpgrade(parsedRequest))
        {

            HttpResponse response = buildWebSocketHandshakeResponse(parsedRequest);

            std::string raw = serializeResponse(response);

            send(client_fd, raw.data(), raw.size(), 0);

            return; // exit the function to stop handling the client in this thread
        }

        // check for http close connection header and close connection if present
        bool closeConnection = parsedRequest.headers.find("Connection") != parsedRequest.headers.end() && parsedRequest.headers.find("Connection")->second.find("close") != std::string::npos;

        HttpResponse response = buildResponse(parsedRequest, directory, closeConnection);
        std::string raw = serializeResponse(response);
        send(client_fd, raw.data(), raw.size(), 0);

        if (closeConnection)
        {
            std::cout << "Closing connection with client FD: " << client_fd << "\n";
            break;
        }
    }

    close(client_fd);

    std::cout << "Worker thread finished for client with FD: " << client_fd << "\n";
}
