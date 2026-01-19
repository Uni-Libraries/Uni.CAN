//
// Includes
//

// ImGUI
#include <imgui.h>

// app
#include "topbar.h"
#include "window_can_connect.h"

namespace APP {
    TopBar::TopBar(const std::shared_ptr<WindowCanConnect>& connect)
        : m_connect(connect)
    {
    }

    bool TopBar::UiUpdate()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, 0.0f));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        if (ImGui::Begin("TopBar", nullptr, flags)) {
            if (auto connect = m_connect.lock()) {
                connect->UiUpdateTopBar();
            }
        }
        ImGui::End();
        return true;
    }
}
