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
        ImGui::SetNextWindowSize({450,250}, ImGuiCond_FirstUseEver);
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

        ImGui::Separator();

        ImGui::SetNextItemWidth(200);
        ImGui::InputUInt("Transmit throttle ms",&m_state.CanMgr().TransmitThrottle(),1,1);

        ImGui::End();

        return true;
    }

    void WindowCanConnect::uiUpdateBackend()
    {
        ImGui::Separator();
        if (ImGui::EnumCombo("Parse backend", m_backend, 200))
        {
            // Make other windows react immediately (even in STANDBY).
            m_state.CanMgr().BackendConfiguredGet() = m_backend;
        }

        if (m_backend != ParseBackend::ProtoPlexer)
        {
            return;
        }

        ImGui::SetNextItemWidth(200);
        if (ImGui::InputUInt("PP own addr (12-bit)", &m_pp_own_address, 1, 16, ImGuiInputTextFlags_CharsHexadecimal))
        {
            auto& cfg = m_state.CanMgr().ProtoPlexerConfigConfiguredGet();
            cfg.own_address = static_cast<uint16_t>(m_pp_own_address & 0x0FFFu);
        }
        m_pp_own_address &= 0x0FFFu;

        ImGui::SetNextItemWidth(200);
        if (ImGui::InputUInt("PP max channels", &m_pp_max_channels, 1, 1))
        {
            auto& cfg = m_state.CanMgr().ProtoPlexerConfigConfiguredGet();
            cfg.max_channels = static_cast<uint16_t>(m_pp_max_channels == 0 ? 1 : m_pp_max_channels);
        }
        if (m_pp_max_channels == 0)
        {
            m_pp_max_channels = 1;
        }

        if (ImGui::Checkbox("PP monitoring (accept all dst)", &m_pp_monitoring))
        {
            auto& cfg = m_state.CanMgr().ProtoPlexerConfigConfiguredGet();
            cfg.monitoring = m_pp_monitoring;
        }
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
