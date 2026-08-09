#include "websocket.hpp"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <array>
#include <sstream>
#include <sys/socket.h>
#include <iostream>
#include <cstdint>
#include "websocket_frame.hpp"
#include <arpa/inet.h>

constexpr int BASE64_SHA1_SIZE = 32;
constexpr const char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

void readHeader(int &client_fd, WebSocketFrame &frame)
{
    uint8_t header[2];

    ssize_t bytes = recv(client_fd, header, 2, 0);

    frame.fin = (header[0] >> 7) & 1;
    frame.opcode = header[0] & 0x0F;
    frame.masked = (header[1] >> 7) & 1;
    frame.payloadLength = header[1] & 0x7F;
}

void readExtendedLen(int &client_fd, WebSocketFrame &frame)
{
    if (frame.payloadLength <= 125)
        return;

    if (frame.payloadLength == 126)
    {
        uint16_t length;

        recv(client_fd, &length, sizeof(length), 0);

        frame.payloadLength = ntohs(length);

        return;
    }

    uint64_t length;

    recv(client_fd, &length, sizeof(length), 0);

    uint8_t *bytes = reinterpret_cast<uint8_t *>(&length);

    frame.payloadLength =
        (static_cast<uint64_t>(bytes[0]) << 56) |
        (static_cast<uint64_t>(bytes[1]) << 48) |
        (static_cast<uint64_t>(bytes[2]) << 40) |
        (static_cast<uint64_t>(bytes[3]) << 32) |
        (static_cast<uint64_t>(bytes[4]) << 24) |
        (static_cast<uint64_t>(bytes[5]) << 16) |
        (static_cast<uint64_t>(bytes[6]) << 8) |
        (static_cast<uint64_t>(bytes[7]));
}

void readMaskingKey(int &client_fd, WebSocketFrame &frame)
{

    recv(client_fd, frame.maskingKey.data(), frame.maskingKey.size(), 0);
}

void readPayload(int &client_fd, WebSocketFrame &frame)
{
    frame.payload.resize(frame.payloadLength);

    recv(client_fd, frame.payload.data(), frame.payload.size(), 0);
}

void unmaskPayload(WebSocketFrame &frame)
{
    for (size_t i = 0; i < frame.payload.size(); i++)
    {
        frame.payload[i] ^= frame.maskingKey[i % 4];
    }
}

void sendFrame(int client_fd, uint8_t opcode, const std::vector<uint8_t> &payload)
{
    uint8_t header[2];

    header[0] = 0x80 | opcode;
    
    uint64_t length = payload.size();

    if(length <= 125)
    {
        header[1] = length;
        send(client_fd, header, 2, 0);
    }
    else if (length <= 65535)
    {
        header[1] = 126;
        uint16_t extendedLen = htons(length);
        send(client_fd, header, 2, 0);
        send(client_fd, &extendedLen, sizeof(extendedLen), 0);
    }
    else
    {
        header[1] = 127;
        uint8_t extendedLength[8];

        extendedLength[0] = (length >> 56) & 0xFF;
        extendedLength[1] = (length >> 48) & 0xFF;
        extendedLength[2] = (length >> 40) & 0xFF;
        extendedLength[3] = (length >> 32) & 0xFF;
        extendedLength[4] = (length >> 24) & 0xFF;
        extendedLength[5] = (length >> 16) & 0xFF;
        extendedLength[6] = (length >> 8) & 0xFF;
        extendedLength[7] = length & 0xFF;

        send(client_fd, header, 2, 0);
        send(client_fd, extendedLength, sizeof(extendedLength), 0);
    }
    

    if (!payload.empty())
    {
        send(
            client_fd,
            payload.data(),
            payload.size(),
            0);
    }
}

WebSocketFrame readFrame(int client_fd)
{
    WebSocketFrame frame;

    readHeader(client_fd, frame);
    readExtendedLen(client_fd, frame);
    if (frame.masked)
        readMaskingKey(client_fd, frame);
    readPayload(client_fd, frame);

    if (frame.masked)
        unmaskPayload(frame);

    return frame;
}

void handleFrame(int client_fd, const WebSocketFrame &frame)
{
    uint8_t opcode = frame.opcode;

    switch (opcode)
    {
    case 0x1: // text frame
        std::cout << "Received text frame: " << std::string(frame.payload.begin(), frame.payload.end()) << std::endl;
        sendFrame(client_fd, opcode, frame.payload);
        break;
    case 0x2: // binary frame
        std::cout << "Received binary frame of size: " << frame.payload.size() << std::endl;
        break;
    case 0x8: // connection close
        std::cout << "Received close frame" << std::endl;
        return;
    case 0x9: // ping
        std::cout << "Received ping frame" << std::endl;
        sendFrame(client_fd, 0xA, frame.payload); // send pong
        return;
    case 0xA: // pong
        std::cout << "Received pong frame" << std::endl;
        return;
    default:
        std::cerr << "Unknown opcode: " << static_cast<int>(opcode) << std::endl;
        return;
    }
}

HttpResponse buildWebSocketHandshakeResponse(const HttpRequest &request)
{
    HttpResponse response;

    response.version = "HTTP/1.1";
    response.statusCode = 101;
    response.statusMessage = "Switching Protocols";

    response.headers["Upgrade"] = "websocket";
    response.headers["Connection"] = "Upgrade";
    response.headers["Sec-WebSocket-Accept"] = generateWebSocketAcceptKey(request.headers.at("Sec-WebSocket-Key"));

    response.body = "";

    return response;
}

std::string generateWebSocketAcceptKey(const std::string &clientKey)
{
    // append guid
    std::string key = clientKey + GUID;
    // sha1
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(key.data()), key.size(), hash);

    // base64
    unsigned char output[BASE64_SHA1_SIZE];
    EVP_EncodeBlock(output, hash, SHA_DIGEST_LENGTH);
    // return string
    return std::string(reinterpret_cast<char *>(output));
}

bool isWebSocketUpgrade(const HttpRequest &parsedRequest)
{
    if (
        parsedRequest.method == "GET" &&
        parsedRequest.headers.count("Upgrade") &&
        parsedRequest.headers.find("Upgrade")->second == "websocket" &&
        parsedRequest.headers.count("Connection") &&
        parsedRequest.headers.find("Connection")->second == "Upgrade" &&
        parsedRequest.headers.count("Sec-WebSocket-Key") &&
        parsedRequest.headers.count("Sec-WebSocket-Version") &&
        parsedRequest.headers.find("Sec-WebSocket-Version")->second == "13")
        return true;

    return false;
}

void handleWebSocket(int client_fd)
{
    while (true)
    {
        WebSocketFrame frame = readFrame(client_fd);

        handleFrame(client_fd, frame);
    }
}