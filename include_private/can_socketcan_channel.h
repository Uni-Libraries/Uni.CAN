#pragma once

// stdlib
#include <cstdint>
#include <memory>

// Uni.CAN
#include "uni_can_devinfo.h"
#include "can_channel_interface.h"


//
// Public
//

namespace Uni::CAN {
    class CanChannelSocketcan : public ICanChannel {
    public:
        ~CanChannelSocketcan() override;

        bool Init() override;

        bool DeInit() override;

        bool Open() override;

        bool Close() override;

        [[nodiscard]] uni_can_message_t* ReceiveMessage() override;

        bool TransmitMessage(const uni_can_message_t &msg) override;

    protected:
        friend class CanProviderSocketcan;

        explicit CanChannelSocketcan(const uni_can_devinfo_t *devInfo, int channel_idx, uint32_t baudrate);

    private:
        uni_can_devinfo_t _dev_info{};
        uint32_t _can_baudrate{0};
        uint32_t _can_restart{100};

        int _fd = -1;
    };
} // namespace Uni::CAN
