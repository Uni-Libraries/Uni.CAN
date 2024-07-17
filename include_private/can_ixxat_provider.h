#pragma once

// auris.can --> public
#include "can_provider.h"

// auris.can --> private
#include "uni_can_devinfo.h"

namespace Auris::CAN {
    class CanProviderIxxat : public ICanProvider {
      public:
        void Init() override;

        bool IsInited() override;

        ICanChannel* CreateChannel(uni_can_devinfo_t* devInfo, size_t channelIdx, uint32_t baudrate) override;

        std::vector<std::shared_ptr<uni_can_devinfo_t>> GetDeviceInfo() override;

      private:
        bool _inited = false;
    };
} // namespace Auris::CAN
