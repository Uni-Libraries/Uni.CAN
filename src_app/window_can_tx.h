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
    class WindowCanTx: public Uni::GUI::UiElement {
    public:
        explicit WindowCanTx(State& state) : m_state(state) {}
        bool UiUpdate() override;

    private:
        uni_can_message_t m_msg{};

        // protoplexer
        unsigned int m_pp_msg_id{0x0001};
        unsigned int m_pp_to{0x001};
        unsigned int m_pp_from{0x001};
        unsigned int m_pp_prio{0x07};
        std::string m_pp_payload_hex;

        State& m_state;
    };
}
