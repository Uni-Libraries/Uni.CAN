#pragma once

// stdlib
#include <memory>
#include <vector>

// auris.can
#include "uni_can_devinfo.h"

// uni.can.private
#include "can_channel_interface.h"



namespace Auris::CAN {
    class ICanProvider {
      public:
        virtual void Init() = 0;

        virtual bool IsInited() = 0;

        virtual ICanChannel* CreateChannel(uni_can_devinfo_t* devInfo, size_t channelIdx, uint32_t baudrate) = 0;

        virtual std::vector<std::shared_ptr<uni_can_devinfo_t>> GetDeviceInfo() = 0;
    };
}
