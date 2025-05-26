#pragma once

// stdlib
#include <cstdint>

// Uni.CAN
#include "can_channel_base.h"
#include "uni_can_devinfo.h"


//
// Public
//

namespace Uni::CAN {
    class CanChannelSocketcan: public CanChannelBase {
        //ctor
    public:
        CanChannelSocketcan(const uni_can_devinfo_t *devInfo, int channel_idx, uint32_t baudrate);
        ~CanChannelSocketcan() override;

        //ICanChannel
    public:
        bool Init() override;
        bool DeInit() override;
        bool Open() override;
        bool Close() override;
        [[nodiscard]] uni_can_message_t* ReceiveMessage() override;
        bool TransmitMessage(const uni_can_message_t &msg) override;

        //thread
    private:
        void threadProc() override;
        bool threadProcReceive();
        bool threadStop() override;
    private:
        int _thread_fd = -1;

    private:
        uint32_t _can_restart{100};
        int _fd = -1;

    };
} // namespace Uni::CAN
