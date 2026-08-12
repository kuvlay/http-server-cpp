#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <openssl/ssl.h>

#include "../http/http_types.hpp"
#include "websocket_frame.hpp"

bool recvAll(
    SSL *ssl,
    void *buffer,
    size_t length);

bool sendAll(
    SSL *ssl,
    const void *buffer,
    size_t length);

bool readHeader(
    SSL *ssl,
    WebSocketFrame &frame);

bool readExtendedLen(
    SSL *ssl,
    WebSocketFrame &frame);

bool readMaskingKey(
    SSL *ssl,
    WebSocketFrame &frame);

bool readPayload(
    SSL *ssl,
    WebSocketFrame &frame);

void unmaskPayload(
    WebSocketFrame &frame);

bool readFrame(
    SSL *ssl,
    WebSocketFrame &frame);

bool sendFrame(
    SSL *ssl,
    uint8_t opcode,
    const std::vector<uint8_t> &payload);

bool handleFrame(
    SSL *ssl,
    const WebSocketFrame &frame);

HttpResponse buildWebSocketHandshakeResponse(
    const HttpRequest &request);

std::string generateWebSocketAcceptKey(
    const std::string &clientKey);

bool isWebSocketUpgrade(
    const HttpRequest &parsedRequest);

void handleWebSocket(SSL *ssl);