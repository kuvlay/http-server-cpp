#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "../http/http_types.hpp"
#include "websocket_frame.hpp"

bool recvAll(
    int client_fd,
    void *buffer,
    size_t length);

bool sendAll(
    int client_fd,
    const void *buffer,
    size_t length);

bool readHeader(
    int client_fd,
    WebSocketFrame &frame);

bool readExtendedLen(
    int client_fd,
    WebSocketFrame &frame);

bool readMaskingKey(
    int client_fd,
    WebSocketFrame &frame);

bool readPayload(
    int client_fd,
    WebSocketFrame &frame);

void unmaskPayload(
    WebSocketFrame &frame);

bool readFrame(
    int client_fd,
    WebSocketFrame &frame);

bool sendFrame(
    int client_fd,
    uint8_t opcode,
    const std::vector<uint8_t> &payload);

bool handleFrame(
    int client_fd,
    const WebSocketFrame &frame);

HttpResponse buildWebSocketHandshakeResponse(
    const HttpRequest &request);

std::string generateWebSocketAcceptKey(
    const std::string &clientKey);

bool isWebSocketUpgrade(
    const HttpRequest &parsedRequest);

void handleWebSocket(int client_fd);