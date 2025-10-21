#pragma once

// uni.CAN
#include "uni_can_channel.h"
#include "uni_can_devinfo.h"
#include "can_channel_base.h"

namespace Uni::CAN {
    class CanChannelMarathon : public CanChannelBase {
        //ctor
    public:
        explicit CanChannelMarathon(uni_can_devinfo_t *devInfo, size_t channelIdx, uint32_t baudrate);
        ~CanChannelMarathon() override;

        //ICanChannel
    public:
        bool Init() override;
        bool DeInit() override;
        bool Open() override;
        bool Close() override;
        bool TransmitMessage(const uni_can_message_t &msg) override;

        // thread
    private:
        void threadProc() override;
        bool threadProcReceive();
    };
} // namespace Uni::CAN
