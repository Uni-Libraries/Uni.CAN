#pragma once

// stdlib
#include <cstdint>
#include <memory>

// auris.can --> public
#include "uni_can_devinfo.h"

// auris.can --> private
#include "can_channel_interface.h"



//
// Public
//

namespace Auris::CAN {
    class CanChannelSocketcan : public ICanChannel {
      public:
        ~CanChannelSocketcan() override;

        bool Init() override;
        bool DeInit() override;

        bool Open() override;
        bool Close() override;

        bool ReceiveMessage(uni_can_message_t &msg) override;
        bool TransmitMessage(const uni_can_message_t &msg) override;

      protected:
        friend class CanProviderSocketcan;
        explicit CanChannelSocketcan(uni_can_devinfo_t* devInfo, int channel_idx, uint32_t baudrate);

      private:
        uni_can_devinfo_t _dev_info;
        uint32_t _can_baudrate = 0;

        int _fd = -1;
    };
} // namespace Auris::CAN
