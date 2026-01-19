#pragma once

// stdlib
#include <memory>

// Uni.GUI
#include "ui_element.h"

namespace APP {
    class WindowCanConnect;

    class TopBar : public Uni::GUI::UiElement {
    public:
        explicit TopBar(const std::shared_ptr<WindowCanConnect>& connect);
        bool UiUpdate() override;

    private:
        std::weak_ptr<WindowCanConnect> m_connect;
    };
}
