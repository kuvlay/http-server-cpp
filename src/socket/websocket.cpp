#include "websocket.hpp"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <array>

constexpr int BASE64_SHA1_SIZE = 32;
constexpr const char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

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

std::string generateWebSocketAcceptKey(
    const std::string &clientKey)
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
