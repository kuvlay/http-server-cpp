#pragma once

#include <string>
#include "http_types.hpp"

HttpResponse handleRoot(bool shouldClose);
HttpResponse handleEcho(const HttpRequest &request, bool shouldClose);
HttpResponse handleUserAgent(const HttpRequest &request, bool shouldClose);
HttpResponse handleGetFile(const HttpRequest &request, const std::string &directory, bool shouldClose);
HttpResponse handlePostFile(const HttpRequest &request, const std::string &directory, bool shouldClose);
HttpResponse handleNotFound(bool shouldClose);