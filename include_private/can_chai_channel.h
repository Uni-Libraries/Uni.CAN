#pragma once

// stdlib
#include <cstdint>
#include <memory>

// uni.CAN
#include "uni_can_channel.h"
#include "uni_can_devinfo.h"
#include "can_channel_interface.h"

namespace Uni::CAN {
    class CanChannelChai : public ICanChannel {
    public:
        ~CanChannelChai() override;

        bool Init() override;

        bool DeInit() override;

        bool Open() override;

        bool Close() override;

        bool ReceiveMessage(uni_can_message_t &msg) override;

        bool TransmitMessage(const uni_can_message_t &msg) override;

    protected:
        friend class CanProviderChai;

        explicit CanChannelChai(uni_can_devinfo_t *devInfo, size_t channelIdx, uint32_t baudrate);

    private:
        uint8_t _channel_num = -1;
        uni_can_devinfo_t _dev_info;
        uint32_t _can_baudrate = 0;
    };
} // namespace Uni::CAN
