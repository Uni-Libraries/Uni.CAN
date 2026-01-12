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
    class ProtoPlexerDictionary;

    class WindowCanRx: public Uni::GUI::UiElement {
    public:
        explicit WindowCanRx(State& state);
        ~WindowCanRx() override;

        bool UiUpdate() override;

    private:
        void uiDetails();
        void uiTable();

        void uiTableRawCan();
        void uiTableProtoPlexer();

        void uiFilter();

    private:
        void clear();
        void receiveMsg(const RxPacket& msg);

    private:
        bool m_autoscroll = false;
        bool m_filter = false;
        uint32_t m_filter_value = 0;

        std::vector<CanMessagePtr> m_msgs_raw;
        std::vector<ProtoPlexerMessage> m_msgs_pp;
        State& m_state;

        const ProtoPlexerDictionary* m_pp_dict{nullptr};

        //filter
    private:
        bool filterMatch(uint32_t key) const;
        void filterLoad();
        void filterSave();
    private:
        std::vector<uint32_t> m_filter_list;
        int m_filter_selection{-1};
    };
}
