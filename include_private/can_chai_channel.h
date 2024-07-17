#pragma once

// stdlib
#include <cstdint>
#include <memory>

// auris.can --> public
#include "../include/uni_can_channel.h"

// auris.can --> private
#include "can_chai_devinfo.h"

namespace Auris::CAN {
    class CanChannelChai : public ICanChannel {
      public:
        ~CanChannelChai() override;

        bool Init() override;
        bool DeInit() override;

        bool Open() override;
        bool Close() override;

        bool ReceiveMessage(CanMessage &msg) override;
        bool TransmitMessage(CanMessage &msg) override;

      protected:
        friend class CanProviderChai;
        explicit CanChannelChai(std::shared_ptr<CanDevinfoChai> &devInfo, size_t channelIdx, uint32_t baudrate);

      private:
        uint8_t _channel_num = -1;
        std::shared_ptr<CanDevinfoChai> _dev_info;
        uint32_t _can_baudrate = 0;
    };
} // namespace Auris::CAN
