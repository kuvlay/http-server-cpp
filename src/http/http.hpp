#pragma once

#include <string>
#include "http_types.hpp"

HttpRequest parseRequest(const std::string &request);

HttpResponse buildResponse(const HttpRequest &request, const std::string &directory, bool shouldClose);

void wthread_handleClient(int client_fd, const std::string &directory);

std::string serializeResponse(const HttpResponse &response);
