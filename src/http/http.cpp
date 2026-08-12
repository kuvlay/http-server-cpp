#include "http.hpp"
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include "socket/websocket.hpp"
#include "http_types.hpp"
#include "routes.hpp"
#include <openssl/ssl.h>
#include <openssl/err.h>

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

bool tryExtractCompleteRequest(std::string &receiveBuffer, std::string &request)
{
    std::size_t requestEnd =
        receiveBuffer.find("\r\n\r\n");

    if (requestEnd == std::string::npos)
    {
        // headers are incomplete
        return false;
    }

    // find content length
    std::size_t contentLength = 0;

    std::size_t contentLengthPos =
        receiveBuffer.find("Content-Length:");

    if (contentLengthPos != std::string::npos &&
        contentLengthPos < requestEnd)
    {
        std::size_t valueStart =
            contentLengthPos + 15;

        std::size_t valueEnd =
            receiveBuffer.find("\r\n", valueStart);

        if (valueEnd != std::string::npos)
        {
            contentLength = std::stoul(
                receiveBuffer.substr(
                    valueStart,
                    valueEnd - valueStart));
        }
    }

    std::size_t totalRequestSize =
        requestEnd + 4 + contentLength;

    if (receiveBuffer.size() < totalRequestSize)
    {
        // haders arrived, but body hasn't atleast not completely
        return false;
    }

    // then extract one full request
    request =
        receiveBuffer.substr(
            0,
            totalRequestSize);

    // remove the rquest frmo buffer

    receiveBuffer.erase(
        0,
        totalRequestSize);

    return true;
}

void wthread_handleClient(int client_fd, SSL *ssl, const std::string &directory)
{

    std::cout << "Worker thread started for client with FD: "
              << client_fd << "\n";

    if (SSL_accept(ssl) <= 0)
    {
        std::cerr << "TLS handshake failed\n";

        SSL_free(ssl);
        close(client_fd);

        return;
    }

    std::cout << "TLS handshake successful for client FD: "
              << client_fd << "\n";

    // this survives across multiple SSL_read() calls.
    std::string receiveBuffer;
    bool terminateConnection = false;

    // outer loop-read more bytes from the network

    while (true)
    {
        char buffer[1024];

        int bytes_read =
            SSL_read(
                ssl,
                buffer,
                sizeof(buffer));

        std::cout
            << "data received from client: "
            << client_fd
            << ": "
            << bytes_read
            << "\n";

        if (bytes_read <= 0)
        {
            int ssl_error =
                SSL_get_error(
                    ssl,
                    bytes_read);

            if (ssl_error == SSL_ERROR_ZERO_RETURN)
            {
                std::cout
                    << "TLS connection closed by client\n";
            }
            else
            {
                std::cerr
                    << "SSL_read failed, error: "
                    << ssl_error
                    << "\n";
            }

            break;
        }

        receiveBuffer.append(
            buffer,
            bytes_read);

        // inner loop- process each &every complete request currently

        while (true)
        {
            std::string request;

            // get one complete request
            bool completeRequest =
                tryExtractCompleteRequest(
                    receiveBuffer,
                    request);

            if (!completeRequest)
            {
                break;
            }

            // parse nd store in httprequest structure
            HttpRequest parsedRequest =
                parseRequest(request);

            // websocket connection chekc
            if (isWebSocketUpgrade(parsedRequest))
            {
                HttpResponse response =
                    buildWebSocketHandshakeResponse(
                        parsedRequest);

                std::string raw =
                    serializeResponse(response);

                // reliably send handshake response
                size_t totalSent = 0;

                while (totalSent < raw.size())
                {
                    int bytesSent =
                        SSL_write(
                            ssl,
                            raw.data() + totalSent,
                            raw.size() - totalSent);

                    if (bytesSent <= 0)
                    {
                        int ssl_error =
                            SSL_get_error(
                                ssl,
                                bytesSent);

                        std::cerr
                            << "SSL_write failed, error: "
                            << ssl_error
                            << "\n";

                        break;
                    }

                    totalSent += bytesSent;
                }

                // we sent 101 response now swithcing protocol/ws
                handleWebSocket(ssl);
                terminateConnection = true;

                break;
            }

            // check close http
            bool closeConnection =
                parsedRequest.headers.find("Connection") != parsedRequest.headers.end() &&
                parsedRequest.headers
                        .find("Connection")
                        ->second
                        .find("close") != std::string::npos;

            // build http reaonse
            HttpResponse response =
                buildResponse(
                    parsedRequest,
                    directory,
                    closeConnection);

            std::string raw =
                serializeResponse(response);

            /// ssl write
            size_t totalSent = 0;

            while (totalSent < raw.size())
            {
                int bytesSent =
                    SSL_write(
                        ssl,
                        raw.data() + totalSent,
                        raw.size() - totalSent);

                if (bytesSent <= 0)
                {
                    int ssl_error =
                        SSL_get_error(
                            ssl,
                            bytesSent);

                    std::cerr
                        << "SSL_write failed, error: "
                        << ssl_error
                        << "\n";

                    break;
                }

                totalSent += bytesSent;
            }

            // since our logic states if close conneciton we already sent a close conneciton response
            // now close the connection
            if (closeConnection)
            {
                std::cout
                    << "Closing connection with client FD: "
                    << client_fd
                    << "\n";

                break;
            }
        }
        if (terminateConnection)
        {
            break;
        }
    }

    // cleanup
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(client_fd);

    std::cout
        << "Worker thread finished for client with FD: "
        << client_fd
        << "\n";
}
