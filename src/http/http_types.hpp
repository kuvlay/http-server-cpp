#pragma once

#include <string>
#include <map>
#include <unordered_map>

struct HttpRequest
{
    std::string method;
    std::string path;
    std::string version;

    std::unordered_map<std::string, std::string> headers;

    std::string body;
};

struct HttpResponse
{
    std::string version;
    int statusCode;
    std::string statusMessage;

    std::map<std::string, std::string> headers;

    std::string body;
};