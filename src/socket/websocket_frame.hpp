#pragma once

#include <cstdint>
#include <vector>
#include <array>

struct WebSocketFrame
{
    bool fin;

    uint8_t opcode;

    bool masked;

    uint64_t payloadLength;

    std::array<uint8_t, 4> maskingKey;

    std::vector<uint8_t> payload;
};