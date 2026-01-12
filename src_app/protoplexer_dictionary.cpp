// ProtoPlexer dictionaries (message-id and address names + optional field descriptors)

// stdlib
#include <filesystem>
#include <fstream>

// fmt
#include <fmt/format.h>

// nlohmann
#include <nlohmann/json.hpp>

// app
#include "protoplexer_dictionary.h"

namespace APP {
    namespace {
        static std::uint16_t parse_u16(const nlohmann::json& j)
        {
            if (j.is_number_unsigned()) {
                return static_cast<std::uint16_t>(j.get<std::uint32_t>() & 0xFFFFu);
            }
            if (j.is_number_integer()) {
                return static_cast<std::uint16_t>(static_cast<std::uint32_t>(j.get<std::int64_t>()) & 0xFFFFu);
            }
            if (j.is_string()) {
                const std::string s = j.get<std::string>();
                const unsigned long v = std::stoul(s, nullptr, 0 /* base autodetect (0x.. supported) */);
                return static_cast<std::uint16_t>(v & 0xFFFFu);
            }
            return 0;
        }

        static std::uint32_t parse_u32(const nlohmann::json& j)
        {
            if (j.is_number_unsigned()) {
                return j.get<std::uint32_t>();
            }
            if (j.is_number_integer()) {
                return static_cast<std::uint32_t>(j.get<std::int64_t>());
            }
            if (j.is_string()) {
                const std::string s = j.get<std::string>();
                const unsigned long v = std::stoul(s, nullptr, 0);
                return static_cast<std::uint32_t>(v);
            }
            return 0;
        }

        static std::int32_t parse_i32(const nlohmann::json& j)
        {
            if (j.is_number_integer()) {
                return static_cast<std::int32_t>(j.get<std::int64_t>());
            }
            if (j.is_number_unsigned()) {
                return static_cast<std::int32_t>(j.get<std::uint64_t>());
            }
            if (j.is_string()) {
                const std::string s = j.get<std::string>();
                const long v = std::stol(s, nullptr, 0);
                return static_cast<std::int32_t>(v);
            }
            return 0;
        }

        static bool read_u32le_at(const std::vector<std::uint8_t>& data, std::size_t off, std::uint32_t* out)
        {
            if (!out) {
                return false;
            }
            if (off + 4 > data.size()) {
                return false;
            }
            *out = (std::uint32_t)data[off] |
                   ((std::uint32_t)data[off + 1] << 8U) |
                   ((std::uint32_t)data[off + 2] << 16U) |
                   ((std::uint32_t)data[off + 3] << 24U);
            return true;
        }

        static bool read_u8_at(const std::vector<std::uint8_t>& data, std::size_t off, std::uint8_t* out)
        {
            if (!out) {
                return false;
            }
            if (off + 1 > data.size()) {
                return false;
            }
            *out = data[off];
            return true;
        }
    }

    void ProtoPlexerDictionary::clear()
    {
        m_messages.clear();
        m_addresses.clear();
    }

    bool ProtoPlexerDictionary::TryLoadFromFile(const std::string& path, std::string* out_error)
    {
        // No defaults: dictionary must come from JSON.
        if (path.empty()) {
            if (out_error) {
                *out_error = "empty path";
            }
            return false;
        }

        try {
            if (!std::filesystem::exists(path)) {
                if (out_error) {
                    *out_error = "file does not exist";
                }
                return false;
            }

            std::ifstream f(path);
            if (!f.is_open()) {
                if (out_error) {
                    *out_error = "failed to open file";
                }
                return false;
            }

            nlohmann::json j;
            f >> j;

            const nlohmann::json* root = &j;
            if (j.is_object() && j.contains("protoplexer")) {
                root = &j.at("protoplexer");
            }

            if (!root->is_object()) {
                if (out_error) {
                    *out_error = "root is not an object";
                }
                return false;
            }

            // Load ONLY from JSON.
            clear();

            // messages
            if (root->contains("message_ids")) {
                const auto& msg = root->at("message_ids");

                if (msg.is_array()) {
                    for (const auto& item : msg) {
                        if (!item.is_object() || !item.contains("id") || !item.contains("name")) {
                            continue;
                        }

                        ProtoPlexerMessageDef def{};
                        def.id = parse_u16(item.at("id"));
                        def.name = item.at("name").get<std::string>();

                        if (item.contains("fields") && item.at("fields").is_array()) {
                            for (const auto& fj : item.at("fields")) {
                                if (!fj.is_object() || !fj.contains("name")) {
                                    continue;
                                }
                                ProtoPlexerField fdef{};
                                fdef.name = fj.at("name").get<std::string>();
                                fdef.type = fj.value("type", "");
                                if (fj.contains("offset")) {
                                    fdef.offset = parse_u32(fj.at("offset"));
                                }
                                if (fj.contains("size")) {
                                    fdef.size = parse_u32(fj.at("size"));
                                }
                                if (fj.contains("bit")) {
                                    fdef.bit = parse_i32(fj.at("bit"));
                                }
                                def.fields.push_back(std::move(fdef));
                            }
                        }

                        m_messages[def.id] = std::move(def);
                    }
                }
                else if (msg.is_object()) {
                    for (auto it = msg.begin(); it != msg.end(); ++it) {
                        ProtoPlexerMessageDef def{};
                        def.id = static_cast<std::uint16_t>(std::stoul(it.key(), nullptr, 0) & 0xFFFFu);
                        if (!it.value().is_object()) {
                            continue;
                        }
                        def.name = it.value().value("name", "");
                        m_messages[def.id] = std::move(def);
                    }
                }
            }

            // addresses
            if (root->contains("addresses")) {
                const auto& addr = root->at("addresses");
                if (addr.is_array()) {
                    for (const auto& item : addr) {
                        if (!item.is_object() || !item.contains("addr") || !item.contains("name")) {
                            continue;
                        }
                        const auto a = parse_u16(item.at("addr"));
                        const auto name = item.at("name").get<std::string>();
                        m_addresses[a] = name;
                    }
                }
                else if (addr.is_object()) {
                    for (auto it = addr.begin(); it != addr.end(); ++it) {
                        const auto a = static_cast<std::uint16_t>(std::stoul(it.key(), nullptr, 0) & 0xFFFFu);
                        if (it.value().is_string()) {
                            m_addresses[a] = it.value().get<std::string>();
                        }
                    }
                }
            }

            return true;
        }
        catch (const std::exception& e) {
            if (out_error) {
                *out_error = e.what();
            }
            return false;
        }
    }

    std::string_view ProtoPlexerDictionary::MessageName(std::uint16_t id) const
    {
        const auto it = m_messages.find(id);
        if (it == m_messages.end()) {
            return {};
        }
        return it->second.name;
    }

    std::string_view ProtoPlexerDictionary::AddressName(std::uint16_t addr) const
    {
        const auto it = m_addresses.find(addr);
        if (it == m_addresses.end()) {
            return {};
        }
        return it->second;
    }

    std::string ProtoPlexerDictionary::FormatMessageId(std::uint16_t id) const
    {
        const auto n = MessageName(id);
        if (!n.empty()) {
            return fmt::format("{:04X} ({})", id, n);
        }
        return fmt::format("{:04X}", id);
    }

    std::string ProtoPlexerDictionary::FormatAddress(std::uint16_t addr) const
    {
        const auto n = AddressName(addr);
        if (!n.empty()) {
            return fmt::format("{:03X} ({})", addr & 0x0FFFu, n);
        }
        return fmt::format("{:03X}", addr & 0x0FFFu);
    }

    std::string ProtoPlexerDictionary::FormatPayloadSummary(std::uint16_t id, const std::vector<std::uint8_t>& data) const
    {
        const auto it = m_messages.find(id);
        if (it == m_messages.end()) {
            return {};
        }

        const auto& def = it->second;
        if (def.fields.empty()) {
            return {};
        }

        std::string out;
        bool first = true;

        auto append_kv = [&](const std::string& k, const std::string& v) {
            if (!first) {
                out.push_back(' ');
            }
            first = false;
            out += k;
            out.push_back('=');
            out += v;
        };

        for (const auto& f : def.fields) {
            if (f.type == "u8") {
                std::uint8_t v = 0;
                if (!read_u8_at(data, f.offset, &v)) {
                    continue;
                }
                append_kv(f.name, fmt::format("{}", (unsigned)v));
            }
            if (f.type == "u32le") {
                std::uint32_t v = 0;
                if (!read_u32le_at(data, f.offset, &v)) {
                    continue;
                }
                append_kv(f.name, fmt::format("{}", v));
            }
            else if (f.type == "bit") {
                std::uint8_t b = 0;
                if (!read_u8_at(data, f.offset, &b)) {
                    continue;
                }
                if (f.bit < 0 || f.bit > 7) {
                    continue;
                }
                const std::uint8_t v = (b >> (unsigned)f.bit) & 0x01u;
                append_kv(f.name, fmt::format("{}", (unsigned)v));
            }
        }

        return out;
    }

    const ProtoPlexerDictionary& GetProtoPlexerDictionary()
    {
        static ProtoPlexerDictionary dict = []() {
            ProtoPlexerDictionary d;

            // Try a couple convenient locations.
            // If not found, dictionary stays empty (UI will show only HEX values).
            std::string err;
            if (!d.TryLoadFromFile("protoplexer_dict.json", &err)) {
                (void)d.TryLoadFromFile("src_app/protoplexer_dict.json", nullptr);
            }
            return d;
        }();

        return dict;
    }
}
