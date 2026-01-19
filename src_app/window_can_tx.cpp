//
// Includes
//

// ImGUI
#include <imgui.h>

// ImGUI std::string helpers
#include <imgui_stdlib.h>

// Uni.GUI
#include "imgui_adds.h"

// app
#include "window_can_tx.h"

// app
#include "protoplexer_dictionary.h"

// stdlib
#include <cctype>
#include <cstring>
#include <unordered_map>
#include <sstream>



//
// Implementation
//

namespace APP {
    static std::vector<uint8_t> parseHexBytes(const std::string& s)
    {
        std::vector<uint8_t> out;
        std::string hex;
        hex.reserve(s.size());

        for (unsigned char c : s)
        {
            if (std::isxdigit(c))
            {
                hex.push_back((char)c);
            }
        }
        if (hex.size() % 2 != 0)
        {
            // ignore trailing half-byte
            hex.pop_back();
        }

        out.reserve(hex.size() / 2);
        for (size_t i = 0; i < hex.size(); i += 2)
        {
            unsigned int v = 0;
            std::stringstream ss;
            ss << std::hex << hex.substr(i, 2);
            ss >> v;
            out.push_back((uint8_t)v);
        }
        return out;
    }

    struct ProtoPlexerFieldState {
        std::uint64_t value{0};
        bool bool_value{false};
        std::string bytes_hex;
    };

    namespace {
        static std::string hex_from_bytes(const std::vector<std::uint8_t>& data)
        {
            std::string out;
            out.reserve(data.size() * 3);
            for (size_t i = 0; i < data.size(); ++i) {
                char buf[4]{};
                std::snprintf(buf, sizeof(buf), "%02X ", data[i]);
                out += buf;
            }
            if (!out.empty()) {
                out.pop_back();
            }
            return out;
        }

        static void ensure_size(std::vector<std::uint8_t>& data, std::size_t size)
        {
            if (data.size() < size) {
                data.resize(size, 0);
            }
        }

        static void write_u8(std::vector<std::uint8_t>& data, std::size_t off, std::uint8_t v)
        {
            ensure_size(data, off + 1);
            data[off] = v;
        }

        static void write_u16le(std::vector<std::uint8_t>& data, std::size_t off, std::uint16_t v)
        {
            ensure_size(data, off + 2);
            data[off] = (std::uint8_t)(v & 0xFFu);
            data[off + 1] = (std::uint8_t)((v >> 8) & 0xFFu);
        }

        static void write_u32le(std::vector<std::uint8_t>& data, std::size_t off, std::uint32_t v)
        {
            ensure_size(data, off + 4);
            data[off] = (std::uint8_t)(v & 0xFFu);
            data[off + 1] = (std::uint8_t)((v >> 8) & 0xFFu);
            data[off + 2] = (std::uint8_t)((v >> 16) & 0xFFu);
            data[off + 3] = (std::uint8_t)((v >> 24) & 0xFFu);
        }

        static void write_u32be(std::vector<std::uint8_t>& data, std::size_t off, std::uint32_t v)
        {
            ensure_size(data, off + 4);
            data[off] = (std::uint8_t)((v >> 24) & 0xFFu);
            data[off + 1] = (std::uint8_t)((v >> 16) & 0xFFu);
            data[off + 2] = (std::uint8_t)((v >> 8) & 0xFFu);
            data[off + 3] = (std::uint8_t)(v & 0xFFu);
        }

        static void write_bit(std::vector<std::uint8_t>& data, std::size_t off, int bit, bool v)
        {
            if (bit < 0 || bit > 7) {
                return;
            }
            ensure_size(data, off + 1);
            const std::uint8_t mask = (std::uint8_t)(1u << (unsigned)bit);
            if (v) {
                data[off] |= mask;
            } else {
                data[off] &= (std::uint8_t)~mask;
            }
        }

        static std::uint64_t max_value_for_type(const std::string& type)
        {
            if (type == "u8") {
                return 0xFFu;
            }
            if (type == "u16le") {
                return 0xFFFFu;
            }
            if (type == "u32le" || type == "u32be") {
                return 0xFFFFFFFFu;
            }
            return 0xFFFFFFFFu;
        }

        static void update_payload_from_fields(const APP::ProtoPlexerMessageDef* def,
                                               std::unordered_map<std::string, ProtoPlexerFieldState>& state,
                                               std::vector<std::uint8_t>& out_payload)
        {
            out_payload.clear();
            if (!def) {
                return;
            }

            for (const auto& f : def->fields) {
                auto& st = state[f.name];
                if (f.type == "u8") {
                    write_u8(out_payload, f.offset, (std::uint8_t)(st.value & 0xFFu));
                } else if (f.type == "u16le") {
                    write_u16le(out_payload, f.offset, (std::uint16_t)(st.value & 0xFFFFu));
                } else if (f.type == "u32le") {
                    write_u32le(out_payload, f.offset, (std::uint32_t)(st.value & 0xFFFFFFFFu));
                } else if (f.type == "u32be") {
                    write_u32be(out_payload, f.offset, (std::uint32_t)(st.value & 0xFFFFFFFFu));
                } else if (f.type == "bit") {
                    write_bit(out_payload, f.offset, f.bit, st.bool_value);
                } else if (f.type == "bytes") {
                    const auto bytes = parseHexBytes(st.bytes_hex);
                    if (!bytes.empty()) {
                        ensure_size(out_payload, f.offset + bytes.size());
                        std::memcpy(out_payload.data() + f.offset, bytes.data(), bytes.size());
                    }
                }
            }
        }
    }

    bool WindowCanTx::UiUpdate() {
        ImGui::SetNextWindowPos({ 0,260 }, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({ 450,450 }, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("CAN TX")) {

            const bool proto_mode = (m_state.CanMgr().BackendConfiguredGet() == ParseBackend::ProtoPlexer);

            auto ui_raw_can = [&]() {
                ImGui::Text("Raw CAN frame");

                static bool ext_id = true;
                ImGui::Checkbox("EXT ID", &ext_id);

                ImGui::InputUInt("ID", &m_msg.id);
                ImGui::InputUShort("DLC", &m_msg.len);
                if (m_msg.len > 8) {
                    m_msg.len = 8;
                }

                for (size_t idx = 0; idx < sizeof(m_msg.data); idx++) {
                    if (idx > 0) {
                        ImGui::SameLine();
                    }
                    ImGui::SetNextItemWidth(50);

                    ImGui::BeginDisabled(idx >= m_msg.len);
                    ImGui::InputUByte(std::string("##can_data_") + std::to_string(idx), &m_msg.data.u8[idx], 0, 0);
                    ImGui::EndDisabled();
                }

                if (ImGui::Button("Send##send_raw_can")) {
                    m_msg.flags = ext_id ? UNI_CAN_MSG_FLAG_EXT_ID : UNI_CAN_MSG_FLAG_STD_ID;
                    m_state.CanMgr().SendMessage(&m_msg);
                }
            };

            auto ui_pp = [&]() {
                const auto& dict = GetProtoPlexerDictionary();

                ImGui::Text("ProtoPlexer message");

                const auto msg_ids = dict.MessageIds();
                const auto addr_ids = dict.AddressIds();

                ImGui::SetNextItemWidth(260);
                const auto msg_preview = dict.FormatMessageId(static_cast<std::uint16_t>(m_pp_msg_id));
                if (ImGui::BeginCombo("msg_id", msg_preview.empty() ? "----" : msg_preview.c_str())) {
                    if (msg_ids.empty()) {
                        ImGui::TextDisabled("no message_ids in dict");
                    }
                    for (const auto id : msg_ids) {
                        const bool selected = (m_pp_msg_id == id);
                        const auto label = dict.FormatMessageId(id);
                        if (ImGui::Selectable(label.c_str(), selected)) {
                            m_pp_msg_id = id;
                        }
                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::SetNextItemWidth(260);
                const auto from_preview = dict.FormatAddress(static_cast<std::uint16_t>(m_pp_from));
                if (ImGui::BeginCombo("from (12-bit)", from_preview.empty() ? "----" : from_preview.c_str())) {
                    if (addr_ids.empty()) {
                        ImGui::TextDisabled("no addresses in dict");
                    }
                    for (const auto id : addr_ids) {
                        const bool selected = (m_pp_from == id);
                        const auto label = dict.FormatAddress(id);
                        if (ImGui::Selectable(label.c_str(), selected)) {
                            m_pp_from = id;
                        }
                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::SetNextItemWidth(260);
                const auto to_preview = dict.FormatAddress(static_cast<std::uint16_t>(m_pp_to));
                if (ImGui::BeginCombo("to (12-bit)", to_preview.empty() ? "----" : to_preview.c_str())) {
                    if (addr_ids.empty()) {
                        ImGui::TextDisabled("no addresses in dict");
                    }
                    for (const auto id : addr_ids) {
                        const bool selected = (m_pp_to == id);
                        const auto label = dict.FormatAddress(id);
                        if (ImGui::Selectable(label.c_str(), selected)) {
                            m_pp_to = id;
                        }
                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::SetNextItemWidth(200);
                ImGui::InputUInt("priority_inv (0..F)", &m_pp_prio, 1, 1, ImGuiInputTextFlags_CharsHexadecimal);
                m_pp_prio &= 0x0Fu;

                std::vector<std::uint8_t> payload;
                const auto* msg_def = dict.MessageDef(static_cast<std::uint16_t>(m_pp_msg_id));
                static std::unordered_map<std::uint16_t, std::unordered_map<std::string, ProtoPlexerFieldState>> s_pp_field_state;
                auto& field_map = s_pp_field_state[static_cast<std::uint16_t>(m_pp_msg_id)];

                if (msg_def && !msg_def->fields.empty()) {
                    ImGui::Text("Fields");
                    for (const auto& f : msg_def->fields) {
                        auto& st = field_map[f.name];
                        ImGui::PushID(f.name.c_str());
                        if (f.type == "u8" || f.type == "u16le" || f.type == "u32le" || f.type == "u32be") {
                            const auto maxv = max_value_for_type(f.type);
                            ImGui::SetNextItemWidth(200);
                            ImGui::InputUInt64(f.name.c_str(), &st.value, 1, 100, 0);
                            if (st.value > maxv) {
                                st.value = maxv;
                            }
                        } else if (f.type == "bit") {
                            ImGui::Checkbox(f.name.c_str(), &st.bool_value);
                        } else if (f.type == "bytes") {
                            ImGui::SetNextItemWidth(260);
                            ImGui::InputText(f.name.c_str(), &st.bytes_hex);
                        } else {
                            ImGui::TextDisabled("%s (unsupported type: %s)", f.name.c_str(), f.type.c_str());
                        }
                        ImGui::PopID();
                    }

                    update_payload_from_fields(msg_def, field_map, payload);
                    m_pp_payload_hex = hex_from_bytes(payload);
                }
                else {
                    ImGui::Text("payload (hex bytes, any separators)");
                    ImGui::InputTextMultiline("##pp_payload", &m_pp_payload_hex, ImVec2(-FLT_MIN, 80));
                    payload = parseHexBytes(m_pp_payload_hex);
                }

                if (ImGui::Button("Send ProtoPlexer##send_pp"))
                {
                    ProtoPlexerMessage m{};
                    m.message_id = (uint16_t)m_pp_msg_id;
                    m.address_from = (uint16_t)m_pp_from;
                    m.address_to = (uint16_t)m_pp_to;
                    m.priority_inverted = (uint8_t)m_pp_prio;
                    if (!payload.empty()) {
                        m.data = payload;
                    } else {
                        m.data = parseHexBytes(m_pp_payload_hex);
                    }
                    m_state.CanMgr().SendProtoPlexer(m);
                }
            };

            if (proto_mode)
            {
                if (ImGui::BeginTabBar("##tx_tabs"))
                {
                    if (ImGui::BeginTabItem("Raw CAN"))
                    {
                        ui_raw_can();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("ProtoPlexer"))
                    {
                        ui_pp();
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
            }
            else
            {
                ui_raw_can();
            }

            ImGui::End();
        }
        return true;
    }
}
