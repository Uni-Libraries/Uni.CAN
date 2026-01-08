#pragma once

// Types used by the app-level CAN manager/UI.

// stdlib
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

// uni.can
#include <uni_can_message.h>

namespace APP {

    enum class ParseBackend {
        RawCan,
        ProtoPlexer,
    };

    struct ProtoPlexerConfig {
        // Addresses are 12-bit (0x000..0xFFF) in Protoplexer.
        std::uint16_t own_address{0x001};
        std::uint16_t max_channels{16};
        std::uint16_t max_chunk_length{UNI_CAN_MESSAGE_MAXLEN};
        bool monitoring{false};
    };

    struct ProtoPlexerMessage {
        std::uint16_t message_id{};
        std::uint16_t address_from{};
        std::uint16_t address_to{};
        std::uint8_t priority_inverted{0x07};
        std::vector<std::uint8_t> data;
    };

    using CanMessagePtr = std::shared_ptr<uni_can_message_t>; // owned with uni_can_message_free deleter

    using RxPacket = std::variant<CanMessagePtr, ProtoPlexerMessage>;
    using TxPacket = std::variant<CanMessagePtr, ProtoPlexerMessage>;
}
