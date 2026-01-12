//
// Includes
//

// stdlib
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>

// Nlohmann
#include <nlohmann/json.hpp>

// ImGUI
#include <imgui.h>

// Uni.GUI
#include "imgui_adds.h"
#include "window_can_rx.h"

// app
#include "protoplexer_dictionary.h"

#include <iostream>


//
// Implementation
//

namespace APP {
    WindowCanRx::WindowCanRx(State &state) : m_state(state) {
        m_pp_dict = &GetProtoPlexerDictionary();
        filterLoad();

        m_state.CanMgr().ReceiveSubscribe("window_can_rx", [this](const RxPacket& msg) { receiveMsg(msg); });
    }

    WindowCanRx::~WindowCanRx() {
        m_state.CanMgr().ReceiveUnsubscribe("window_can_rx");
    }

    bool WindowCanRx::UiUpdate() {
        ImGui::SetNextWindowPos({ 500,0 }, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({ 950,450 }, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("CAN RX")) {
            if (ImGui::Button("Clear")) {
                clear();
            }
            ImGui::SameLine();
            ImGui::Checkbox("Autoscroll", &m_autoscroll);
            ImGui::SameLine();
            ImGui::Checkbox("Filter", &m_filter);

            uiTable();
        }
        ImGui::End();

        if (m_filter) {
            uiFilter();
        }

        uiDetails();

        return true;
    }

    void WindowCanRx::uiDetails() {
        // details window removed in non-J1939 refactor
    }

    void WindowCanRx::uiTable() {
        if (m_state.CanMgr().BackendConfiguredGet() == ParseBackend::ProtoPlexer)
        {
            if (ImGui::BeginTabBar("##rx_tabs"))
            {
                if (ImGui::BeginTabItem("Raw CAN"))
                {
                    uiTableRawCan();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("ProtoPlexer"))
                {
                    uiTableProtoPlexer();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        else
        {
            uiTableRawCan();
        }
    }

    void WindowCanRx::uiTableRawCan() {
        if (ImGui::BeginChild("ScrollingRegionRaw", ImVec2(0, 0), false,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            if (ImGui::BeginTable("PacketsRaw", 4,
                                  ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersOuter |
                                  ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_None, 0.2f);
                ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_None, 0.15f);
                ImGui::TableSetupColumn("DLC", ImGuiTableColumnFlags_None, 0.1f);
                ImGui::TableSetupColumn("Data", ImGuiTableColumnFlags_None, 0.55f);
                ImGui::TableHeadersRow();

                ImGuiListClipper clipper;
                clipper.Begin((int)m_msgs_raw.size());

                bool process = true;
                while (process && clipper.Step()) {
                    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                        if(i >= (int)m_msgs_raw.size()) {
                            process = false;
                            break;
                        }

                        const auto& ptr = m_msgs_raw[(size_t)i];
                        if (!ptr) {
                            continue;
                        }

                        if(!filterMatch(ptr->id)) {
                            if(i == clipper.DisplayStart) {
                                clipper.DisplayStart++;
                            }
                            clipper.DisplayEnd++;
                            continue;
                        }

                        ImGui::TableNextRow();

                        if (ImGui::TableSetColumnIndex(0)) {
                            ImGui::Text("%08X", ptr->id);
                        }

                        if (ImGui::TableSetColumnIndex(1)) {
                            ImGui::Text("%s", (ptr->flags & UNI_CAN_MSG_FLAG_EXT_ID) ? "EXT" : "STD");
                        }

                        if (ImGui::TableSetColumnIndex(2)) {
                            ImGui::Text("%u", (unsigned)ptr->len);
                        }

                        if (ImGui::TableSetColumnIndex(3)) {
                            std::string data{};
                            for (int idx = 0; idx < (int)ptr->len; idx++) {
                                char buf[8]{};
                                sprintf(buf, "%02X ", ptr->data.u8[idx]);
                                data += buf;
                            }
                            ImGui::Text("%s", data.c_str());
                        }
                    }
                }

                if (m_autoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollHereY(1.0f);
                }

                ImGui::EndTable();
            }
        }
        ImGui::EndChild();
    }

    void WindowCanRx::uiTableProtoPlexer() {
        if (ImGui::BeginChild("ScrollingRegionPP", ImVec2(0, 0), false,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            if (ImGui::BeginTable("PacketsPP", 6,
                                  ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersOuter |
                                  ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupScrollFreeze(0, 1);
                // Wider columns: we now display "HEX (Name)".
                ImGui::TableSetupColumn("MSG_ID", ImGuiTableColumnFlags_None, 0.22f);
                ImGui::TableSetupColumn("From", ImGuiTableColumnFlags_None, 0.15f);
                ImGui::TableSetupColumn("To", ImGuiTableColumnFlags_None, 0.15f);
                ImGui::TableSetupColumn("Prio", ImGuiTableColumnFlags_None, 0.07f);
                ImGui::TableSetupColumn("Len", ImGuiTableColumnFlags_None, 0.07f);
                ImGui::TableSetupColumn("Data", ImGuiTableColumnFlags_None, 0.34f);
                ImGui::TableHeadersRow();

                ImGuiListClipper clipper;
                clipper.Begin((int)m_msgs_pp.size());

                bool process = true;
                while (process && clipper.Step()) {
                    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                        if(i >= (int)m_msgs_pp.size()) {
                            process = false;
                            break;
                        }

                        const auto& m = m_msgs_pp[(size_t)i];
                        if(!filterMatch(m.message_id)) {
                            if(i == clipper.DisplayStart) {
                                clipper.DisplayStart++;
                            }
                            clipper.DisplayEnd++;
                            continue;
                        }

                        ImGui::TableNextRow();

                        if (ImGui::TableSetColumnIndex(0)) {
                            const auto label = m_pp_dict ? m_pp_dict->FormatMessageId(m.message_id) : "";
                            if (!label.empty()) {
                                ImGui::TextUnformatted(label.c_str());
                            } else {
                                ImGui::Text("%04X", m.message_id);
                            }
                        }
                        if (ImGui::TableSetColumnIndex(1)) {
                            const auto label = m_pp_dict ? m_pp_dict->FormatAddress(m.address_from) : "";
                            if (!label.empty()) {
                                ImGui::TextUnformatted(label.c_str());
                            } else {
                                ImGui::Text("%03X", m.address_from);
                            }
                        }
                        if (ImGui::TableSetColumnIndex(2)) {
                            const auto label = m_pp_dict ? m_pp_dict->FormatAddress(m.address_to) : "";
                            if (!label.empty()) {
                                ImGui::TextUnformatted(label.c_str());
                            } else {
                                ImGui::Text("%03X", m.address_to);
                            }
                        }
                        if (ImGui::TableSetColumnIndex(3)) {
                            ImGui::Text("%X", (unsigned)m.priority_inverted);
                        }
                        if (ImGui::TableSetColumnIndex(4)) {
                            ImGui::Text("%u", (unsigned)m.data.size());
                        }
                        if (ImGui::TableSetColumnIndex(5)) {
                            std::string data{};
                            const size_t show = std::min<size_t>(m.data.size(), 16);
                            for (size_t idx = 0; idx < show; idx++) {
                                char buf[8]{};
                                sprintf(buf, "%02X ", m.data[idx]);
                                data += buf;
                            }
                            if (m.data.size() > show) {
                                data += "...";
                            }

                            // Append decoded fields summary (if dictionary has it).
                            if (m_pp_dict) {
                                const auto summary = m_pp_dict->FormatPayloadSummary(m.message_id, m.data);
                                if (!summary.empty()) {
                                    data += " | ";
                                    data += summary;
                                }
                            }
                            ImGui::Text("%s", data.c_str());
                        }
                    }
                }

                if (m_autoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollHereY(1.0f);
                }

                ImGui::EndTable();
            }
        }
        ImGui::EndChild();
    }

    void WindowCanRx::uiFilter() {
        ImGui::SetNextWindowSize({600, 300});
        if (ImGui::Begin("CAN RX Filter", &m_filter)) {
            ImGui::Text("Ignore List");

            int selected_idx = 0;

            if (ImGui::BeginListBox("##filter_listbox", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing()))) {
                for (int n = 0; n < m_filter_list.size(); n++) {
                    const bool is_selected = (m_filter_selection == n);

                    char label[32]{};
                    sprintf(label, "%08X", m_filter_list[n]);
                    if (ImGui::Selectable(label, is_selected)) {
                        m_filter_selection = n;
                    }
                    if (is_selected) {
                        selected_idx = n;
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndListBox();
            }

            if(ImGui::Button("Remove")) {
                m_filter_list.erase(m_filter_list.begin() + selected_idx);
            }


            // add button
            ImGui::Separator();
            ImGui::InputUInt("##input_filter", &m_filter_value, 0, 0, ImGuiInputTextFlags_CharsHexadecimal);
            ImGui::SameLine();
            if (ImGui::Button("Add")) {
                m_filter_list.push_back(m_filter_value);
                filterSave();
            }
        }

        ImGui::End();
    }

    void WindowCanRx::clear() {
        m_msgs_raw.clear();
        m_msgs_pp.clear();
    }

    void WindowCanRx::receiveMsg(const RxPacket& msg) {
        if (std::holds_alternative<CanMessagePtr>(msg)) {
            m_msgs_raw.push_back(std::get<CanMessagePtr>(msg));
        } else {
            m_msgs_pp.push_back(std::get<ProtoPlexerMessage>(msg));
        }
    }

    bool WindowCanRx::filterMatch(uint32_t key) const {
        if(!m_filter) {
            return true;
        }
        return std::find(m_filter_list.begin(), m_filter_list.end(), key) == m_filter_list.end();
    }

    void WindowCanRx::filterLoad() {
        if(std::filesystem::exists("filter.json")) {
            std::ifstream file("filter.json");
            nlohmann::json j;
            file >> j;
            m_filter_list = j.get<std::vector<uint32_t>>();
        }
    }

    void WindowCanRx::filterSave() {

        std::ofstream file("filter.json");
        nlohmann::json j(m_filter_list);
        file << j;
    }
}
