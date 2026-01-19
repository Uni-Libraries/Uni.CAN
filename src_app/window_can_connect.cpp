//
// Includes
//

// ImGUI
#include <imgui.h>

// Uni.GUI
#include "window_can_connect.h"

// app
#include "imgui_adds.h"
#include "can_mgr_state.h"
#include "can_types.h"



//
// Implementation
//

namespace APP {
    //
    // Baudrate
    //

    std::string WindowCanConnect::baudrateGetLabel(size_t idx)
    {
        return std::to_string(m_baudrate[idx]);
    }



    //
    // Device
    //

    std::string WindowCanConnect::deviceGetLabel(size_t idx) {
        std::string str = m_state.CanMgr().InfoGetName(idx);
        if (!str.empty()) {
            return str;
        }
        return "select CAN device";
    }



    //
    // Ui
    //

    bool WindowCanConnect::UiUpdate() {

        ImGui::SetNextWindowPos({0,0}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({450,290}, ImGuiCond_FirstUseEver);
        ImGui::Begin("CAN Connect");

        uiUpdateComboCan();
      
        ImGui::SameLine();
        if(ImGui::Button("Refresh")) {
            m_state.CanMgr().InfoRefresh();
        }

        uiUpdateComboBaudrate();

         uiUpdateBackend();

        if(ImGui::Button("Connect")) {
            CanManager::ConnectParams params{};
            params.device_index = m_device_idx;
            params.baudrate = m_baudrate[m_baudrate_idx];
            params.backend = m_backend;
            params.protoplexer.own_address = static_cast<uint16_t>(m_pp_own_address);
            params.protoplexer.max_channels = static_cast<uint16_t>(m_pp_max_channels);
            params.protoplexer.max_chunk_length = UNI_CAN_MESSAGE_MAXLEN;
            params.protoplexer.monitoring = m_pp_monitoring;
            m_state.CanMgr().Connect(params);
        }
        ImGui::SameLine();

        if(ImGui::Button("Disconnect")) {
            m_state.CanMgr().Disconnect();
        }

        ImGui::SameLine();
        ImGui::Text(to_string(m_state.CanMgr().StateGet()));

         ImGui::End();

        return true;
    }

    bool WindowCanConnect::UiUpdateTopBar()
    {
        const bool connected = m_state.CanMgr().IsConnected();

        ImGui::BeginDisabled(connected);
        ImGui::AlignTextToFramePadding();
        if (ImGui::EnumCombo("Mode", m_backend, 160))
        {
            m_state.CanMgr().BackendConfiguredGet() = m_backend;
        }
        ImGui::EndDisabled();

        if (m_backend != ParseBackend::ProtoPlexer)
        {
            return true;
        }

        ImGui::SameLine();
        ImGui::BeginDisabled(connected);

        ImGui::Text("Address:");
        ImGui::SameLine();

        ImGui::SetNextItemWidth(220);
        if (ImGui::InputUInt(" ##addr", &m_pp_own_address, 1, 16, ImGuiInputTextFlags_CharsHexadecimal))
        {
            auto& cfg = m_state.CanMgr().ProtoPlexerConfigConfiguredGet();
            cfg.own_address = static_cast<uint16_t>(m_pp_own_address & 0x0FFFu);
        }
        m_pp_own_address &= 0x0FFFu;

        ImGui::SameLine();
        if (ImGui::Checkbox("PP monitoring", &m_pp_monitoring))
        {
            auto& cfg = m_state.CanMgr().ProtoPlexerConfigConfiguredGet();
            cfg.monitoring = m_pp_monitoring;
        }

        ImGui::EndDisabled();
        return true;
    }

    void WindowCanConnect::uiUpdateBackend()
    {
        // moved to TopBar
    }

    void WindowCanConnect::uiUpdateComboCan()
    {
        if (ImGui::BeginCombo("##combo_can_device", deviceGetLabel(m_device_idx).c_str())) {

            for (size_t idx = 0; idx < m_state.CanMgr().InfoGetCount(); idx++) {
                const bool is_selected = (m_device_idx == idx);

                if (ImGui::Selectable(deviceGetLabel(idx).c_str(), is_selected)) {
                    m_device_idx = idx;
                }

                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

    }

    void WindowCanConnect::uiUpdateComboBaudrate()
    {
        if (ImGui::BeginCombo("##combo_can_baudrate", baudrateGetLabel(m_baudrate_idx))) {
            for (size_t idx = 0; idx < m_baudrate.size(); idx++) {
                const bool is_selected = (m_baudrate_idx == idx);

                if (ImGui::Selectable(baudrateGetLabel(idx).c_str(), is_selected)) {
                    m_baudrate_idx = idx;
                }

                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }
    }

}
