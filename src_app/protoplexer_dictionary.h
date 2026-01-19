#pragma once

// ProtoPlexer dictionaries (message-id and address names + optional field descriptors)

// stdlib
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace APP {

    struct ProtoPlexerField {
        // Generic field descriptor (payload parsing is application-specific).
        // This project uses it to render a small decoded summary in RX.
        std::string name;
        std::string type;   // e.g. "u8", "u16le", "i32be", "bytes"...
        std::uint32_t offset{0};
        std::uint32_t size{0};  // optional, for "bytes" or fixed arrays

        // For type == "bit": bit index inside the byte at `offset`.
        // Set to -1 if not used.
        int bit{-1};
    };

    struct ProtoPlexerMessageDef {
        std::uint16_t id{0};
        std::string name;
        std::vector<ProtoPlexerField> fields;
    };

    class ProtoPlexerDictionary {
    public:
        // Load/override dictionaries from JSON.
        // Returns false if file exists but couldn't be parsed.
        bool TryLoadFromFile(const std::string& path, std::string* out_error = nullptr);

        std::string_view MessageName(std::uint16_t id) const;
        std::string_view AddressName(std::uint16_t addr) const;

        std::string FormatMessageId(std::uint16_t id) const;  // "0120 (RecordingStarted)"
        std::string FormatAddress(std::uint16_t addr) const;  // "011 (address_storageA)"

        // Returns short human-readable summary for message payload, according to JSON field descriptors.
        // Example: "time_ms=123456" or "antenna_enable=1 transmission_enable=0".
        std::string FormatPayloadSummary(std::uint16_t id, const std::vector<std::uint8_t>& data) const;

        const ProtoPlexerMessageDef* MessageDef(std::uint16_t id) const;
        std::vector<std::uint16_t> MessageIds() const;
        std::vector<std::uint16_t> AddressIds() const;

    private:
        void clear();

        std::unordered_map<std::uint16_t, ProtoPlexerMessageDef> m_messages;
        std::unordered_map<std::uint16_t, std::string> m_addresses;
    };

    // Lazily loads defaults and tries to override them from JSON in a few standard locations.
    const ProtoPlexerDictionary& GetProtoPlexerDictionary();
}
