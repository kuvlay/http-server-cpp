#pragma once

#include <string>
#include "http_types.hpp"
#include <openssl/ssl.h>

HttpRequest parseRequest(const std::string &request);

HttpResponse buildResponse(const HttpRequest &request, const std::string &directory, bool shouldClose);

void wthread_handleClient(int client_fd,SSL *ssl, const std::string &directory);

std::string serializeResponse(const HttpResponse &response);

bool tryExtractCompleteRequest(std::string &receiveBuffer, std::string &request);