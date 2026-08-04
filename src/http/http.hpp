#pragma once

#include <string>
#include "request.hpp"

HttpRequest parseRequest(const std::string &request);

void addHeader(std::string &response, const std::string &key, const std::string &value);
std::string buildResponse(const HttpRequest &request, const std::string &directory, bool shouldClose);
void wthread_handleClient(int client_fd, const std::string &directory);