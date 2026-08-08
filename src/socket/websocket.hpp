#pragma once

#include <string>
#include "http/http.hpp"

bool isWebSocketUpgrade(
    const HttpRequest &parsedRequest);

std::string generateWebSocketAcceptKey(
    const std::string &clientKey);

HttpResponse buildWebSocketHandshakeResponse(
    const HttpRequest &request);