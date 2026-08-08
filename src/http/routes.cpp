#include "routes.hpp"

#include "file/file.hpp"
#include "gzip/gzip.hpp"

namespace
{
    void setCommonHeaders(HttpResponse &response, bool shouldClose)
    {
        if (shouldClose)
            response.headers["Connection"] = "close";

        response.headers["Content-Length"] = std::to_string(response.body.size());
    }
} // namespace

HttpResponse handleRoot(bool shouldClose)
{
    HttpResponse response;
    response.version = "HTTP/1.1";
    response.statusCode = 200;
    response.statusMessage = "OK";
    response.body = "";

    setCommonHeaders(response, shouldClose);
    return response;
}

HttpResponse handleEcho(const HttpRequest &request, bool shouldClose)
{
    HttpResponse response;
    response.version = "HTTP/1.1";
    response.statusCode = 200;
    response.statusMessage = "OK";

    std::string body = request.path.substr(6);
    const auto acceptEncodingHeader = request.headers.find("Accept-Encoding");

    response.headers["Content-Type"] = "text/plain";

    if (acceptEncodingHeader != request.headers.end() && acceptEncodingHeader->second.find("gzip") != std::string::npos)
    {
        response.headers["Content-Encoding"] = "gzip";
        body = gzipCompress(body);
    }

    response.body = body;
    setCommonHeaders(response, shouldClose);
    return response;
}

HttpResponse handleUserAgent(const HttpRequest &request, bool shouldClose)
{
    HttpResponse response;
    response.version = "HTTP/1.1";
    response.statusCode = 200;
    response.statusMessage = "OK";
    response.headers["Content-Type"] = "text/plain";

    const auto userAgentHeader = request.headers.find("User-Agent");
    if (userAgentHeader != request.headers.end())
        response.body = userAgentHeader->second;

    setCommonHeaders(response, shouldClose);
    return response;
}

HttpResponse handleGetFile(const HttpRequest &request, const std::string &directory, bool shouldClose)
{
    HttpResponse response;
    response.version = "HTTP/1.1";

    std::string filename = request.path.substr(7);
    std::string fullPath = directory + filename;

    if (readFile(fullPath, response.body))
    {
        response.statusCode = 200;
        response.statusMessage = "OK";
        response.headers["Content-Type"] = "application/octet-stream";
        setCommonHeaders(response, shouldClose);
        return response;
    }

    response.statusCode = 404;
    response.statusMessage = "Not Found";
    response.body = "";
    setCommonHeaders(response, shouldClose);
    return response;
}

HttpResponse handlePostFile(const HttpRequest &request, const std::string &directory, bool shouldClose)
{
    HttpResponse response;
    response.version = "HTTP/1.1";

    std::string filename = request.path.substr(7);
    std::string fullPath = directory + filename;

    if (writeFile(fullPath, request.body))
    {
        response.statusCode = 201;
        response.statusMessage = "Created";
    }
    else
    {
        response.statusCode = 500;
        response.statusMessage = "Internal Server Error";
    }

    response.body = "";
    setCommonHeaders(response, shouldClose);
    return response;
}

HttpResponse handleNotFound(bool shouldClose)
{
    HttpResponse response;
    response.version = "HTTP/1.1";
    response.statusCode = 404;
    response.statusMessage = "Not Found";
    response.body = "";

    setCommonHeaders(response, shouldClose);
    return response;
}
