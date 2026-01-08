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

// stdlib
#include <cctype>
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
                ImGui::Text("ProtoPlexer message");

                ImGui::SetNextItemWidth(200);
                ImGui::InputUInt("msg_id", &m_pp_msg_id, 1, 16, ImGuiInputTextFlags_CharsHexadecimal);
                m_pp_msg_id &= 0xFFFFu;

                ImGui::SetNextItemWidth(200);
                ImGui::InputUInt("from (12-bit)", &m_pp_from, 1, 16, ImGuiInputTextFlags_CharsHexadecimal);
                m_pp_from &= 0x0FFFu;

                ImGui::SetNextItemWidth(200);
                ImGui::InputUInt("to (12-bit)", &m_pp_to, 1, 16, ImGuiInputTextFlags_CharsHexadecimal);
                m_pp_to &= 0x0FFFu;

                ImGui::SetNextItemWidth(200);
                ImGui::InputUInt("priority_inv (0..F)", &m_pp_prio, 1, 1, ImGuiInputTextFlags_CharsHexadecimal);
                m_pp_prio &= 0x0Fu;

                ImGui::Text("payload (hex bytes, any separators)");
                ImGui::InputTextMultiline("##pp_payload", &m_pp_payload_hex, ImVec2(-FLT_MIN, 80));

                if (ImGui::Button("Send ProtoPlexer##send_pp"))
                {
                    ProtoPlexerMessage m{};
                    m.message_id = (uint16_t)m_pp_msg_id;
                    m.address_from = (uint16_t)m_pp_from;
                    m.address_to = (uint16_t)m_pp_to;
                    m.priority_inverted = (uint8_t)m_pp_prio;
                    m.data = parseHexBytes(m_pp_payload_hex);
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
