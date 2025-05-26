#pragma once

// uni.CAN
#include "uni_can_channel.h"
#include "uni_can_devinfo.h"
#include "can_channel_base.h"
#include "common_queue.h"

namespace Uni::CAN {
    class CanChannelChai : public CanChannelBase {
        //ctor
    public:
        explicit CanChannelChai(uni_can_devinfo_t *devInfo, size_t channelIdx, uint32_t baudrate);
        ~CanChannelChai() override;

        //ICanChannel
    public:
        bool Init() override;
        bool DeInit() override;
        bool Open() override;
        bool Close() override;
        [[nodiscard]] uni_can_message_t* ReceiveMessage() override;
        bool TransmitMessage(const uni_can_message_t &msg) override;

        // thread
    private:
        void threadProc() override;
        bool threadProcReceive();
    };
} // namespace Uni::CAN
