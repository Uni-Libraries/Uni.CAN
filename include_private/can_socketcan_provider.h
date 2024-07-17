#pragma once

// auris.can --> public
#include "can_provider.h"

namespace Auris::CAN {
    class CanProviderSocketcan : public ICanProvider {
      public:
        void Init() override;

        bool IsInited() override;

        ICanChannel* CreateChannel(uni_can_devinfo_t* devInfo, size_t channelIdx, uint32_t baudrate) override;

        std::vector<std::shared_ptr<uni_can_devinfo_t>> GetDeviceInfo() override;

      private:
        bool _inited = false;
    };
} // namespace Auris::CAN