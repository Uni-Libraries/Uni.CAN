#pragma once

//
// Includes
//

// stdlib
#include <chrono>
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

    struct RxRawEntry {
        std::string ts; // HH:MM:SS.mmm
        CanMessagePtr msg;
    };

    struct RxProtoEntry {
        std::string ts; // HH:MM:SS.mmm
        ProtoPlexerMessage msg;
    };

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

        std::vector<RxRawEntry> m_msgs_raw;
        std::vector<size_t> m_msgs_raw_filtered;
        std::vector<RxProtoEntry> m_msgs_pp;
        std::vector<size_t> m_msgs_pp_filtered;
        State& m_state;

        const ProtoPlexerDictionary* m_pp_dict{nullptr};

        //filter
    private:
        bool filterMatch(uint32_t key) const;
        void filterLoad();
        void filterSave();
        void rebuildFilter();
    private:
        std::vector<uint32_t> m_filter_list;
        int m_filter_selection{-1};
    };
}
