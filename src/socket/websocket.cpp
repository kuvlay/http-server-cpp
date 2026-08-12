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
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

constexpr int BASE64_SHA1_SIZE = 32;
constexpr const char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

bool recvAll(SSL *ssl, void *buffer, size_t length)
{
    size_t totalReceived = 0;

    while (totalReceived < length)
    {
        int bytesReceived = SSL_read(
            ssl,
            static_cast<char *>(buffer) + totalReceived,
            length - totalReceived);

        if (bytesReceived <= 0)
        {
            int sslError = SSL_get_error(ssl, bytesReceived);

            if (sslError == SSL_ERROR_ZERO_RETURN)
            {
                std::cout << "TLS connection closed cleanly\n";
            }
            else
            {
                std::cerr << "SSL_read failed, error: "
                          << sslError << "\n";
            }

            return false;
        }

        totalReceived += bytesReceived;
    }

    return true;
}

bool sendAll(SSL *ssl, const void *buffer, size_t length)
{
    size_t totalSent = 0;

    while (totalSent < length)
    {
        int bytesSent = SSL_write(
            ssl,
            static_cast<const char *>(buffer) + totalSent,
            length - totalSent);

        if (bytesSent <= 0)
        {
            int sslError = SSL_get_error(ssl, bytesSent);

            std::cerr << "SSL_write failed, error: "
                      << sslError << "\n";

            return false;
        }

        totalSent += bytesSent;
    }

    return true;
}

bool readHeader(SSL *ssl, WebSocketFrame &frame)
{
    uint8_t header[2];

    if (!recvAll(ssl, header, 2))
        return false;

    frame.fin = (header[0] >> 7) & 1;
    frame.opcode = header[0] & 0x0F;
    frame.masked = (header[1] >> 7) & 1;
    frame.payloadLength = header[1] & 0x7F;
    return true;
}

bool readExtendedLen(SSL *ssl, WebSocketFrame &frame)
{
    if (frame.payloadLength <= 125)
        return true;

    if (frame.payloadLength == 126)
    {
        uint16_t length;

        if (!recvAll(ssl, &length, sizeof(length)))
            return false;

        frame.payloadLength = ntohs(length);

        return true;
    }

    uint64_t length;

    if (!recvAll(ssl, &length, sizeof(length)))
        return false;

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
    return true;
}

bool readMaskingKey(SSL *ssl, WebSocketFrame &frame)
{
    return recvAll(ssl, frame.maskingKey.data(), frame.maskingKey.size());
}

bool readPayload(SSL *ssl, WebSocketFrame &frame)
{
    frame.payload.resize(frame.payloadLength);

    if (frame.payload.empty())
        return true;
    return recvAll(ssl, frame.payload.data(), frame.payload.size());
}

void unmaskPayload(WebSocketFrame &frame)
{
    for (size_t i = 0; i < frame.payload.size(); i++)
        frame.payload[i] ^= frame.maskingKey[i % 4];
}

bool sendFrame(SSL *ssl, uint8_t opcode, const std::vector<uint8_t> &payload)
{
    uint8_t header[2];

    header[0] = 0x80 | opcode;

    uint64_t length = payload.size();

    if (length <= 125)
    {
        header[1] = length;
        if (!sendAll(ssl, header, 2))
            return false;
    }
    else if (length <= 65535)
    {
        header[1] = 126;
        uint16_t extendedLen = htons(length);
        if (!sendAll(ssl, header, 2))
            return false;
        if (!sendAll(ssl, &extendedLen, sizeof(extendedLen)))
            return false;
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

        if (!sendAll(ssl, header, 2))
            return false;

        if (!sendAll(
                ssl,
                extendedLength,
                sizeof(extendedLength)))
            return false;
    }

    if (!payload.empty())
    {
        if (!sendAll(
                ssl,
                payload.data(),
                payload.size()))
            return false;
    }
    return true;
}

bool readFrame(SSL *ssl, WebSocketFrame &frame)
{

    if (!readHeader(ssl, frame))
        return false;
    if (!readExtendedLen(ssl, frame))
        return false;

    if (frame.masked)
        if (!readMaskingKey(ssl, frame))
            return false;

    if (!readPayload(ssl, frame))
        return false;

    if (frame.masked)
        unmaskPayload(frame);

    return true;
}

bool handleFrame(SSL *ssl, const WebSocketFrame &frame)
{
    uint8_t opcode = frame.opcode;

    switch (opcode)
    {
    case 0x1: // text frame
        std::cout << "Received text frame: " << std::string(frame.payload.begin(), frame.payload.end()) << std::endl;
        sendFrame(ssl, opcode, frame.payload);
        break;
    case 0x2: // binary frame
        std::cout << "Received binary frame of size: " << frame.payload.size() << std::endl;
        break;
    case 0x8: // connection close
        std::cout << "Received close frame" << std::endl;
        sendFrame(ssl, 0x8, frame.payload);
        return false;
    case 0x9: // ping
        std::cout << "Received ping frame" << std::endl;
        sendFrame(ssl, 0xA, frame.payload); // send pong
        break;
    case 0xA: // pong
        std::cout << "Received pong frame" << std::endl;
        break;
    default:
        std::cerr << "Unknown opcode: " << static_cast<int>(opcode) << std::endl;
        break;
    }
    return true;
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

void handleWebSocket(SSL *ssl)
{
    while (true)
    {
        WebSocketFrame frame;
        if (!readFrame(ssl, frame))
            break;

        if (!handleFrame(ssl, frame))
            break;
    }
}