#pragma once

//
// Includes
//

// stdlib
#include <cstdint>
#include <string>
#include <vector>

// Uni.GUI
#include "app_state.h"
#include "ui_element.h"

// app
#include "can_types.h"

//
// Public
//

namespace APP {
    class WindowCanConnect: public Uni::GUI::UiElement {
        // ctor
    public:
        explicit WindowCanConnect(State& state) : m_state(state) {}
   

        // baudrate
    private:
        std::string baudrateGetLabel(size_t idx);
    private:
        size_t m_baudrate_idx{2};
        std::vector<int> m_baudrate{ 125'000, 250'000, 500'000, 1'000'000 };


        // device
    private:
        std::string deviceGetLabel(size_t idx);
    private:
        size_t m_device_idx{};

        // backend
    private:
        ParseBackend m_backend{ParseBackend::RawCan};
        unsigned int m_pp_own_address{0x001};
        unsigned int m_pp_max_channels{16};
        bool m_pp_monitoring{false};


        // state
    private:
        State& m_state;

        //ui 
    public:
        bool UiUpdate() override;
    private:
        void uiUpdateComboCan();
        void uiUpdateComboBaudrate();

        void uiUpdateBackend();

    };
}
