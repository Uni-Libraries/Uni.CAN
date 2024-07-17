#pragma once

#if defined(_WIN32)

// auris.can --> public
#include "auris/can/can_devinfo.h"
#include "auris/can/can_provider.h"

// auris.can --> private
#include "can_chai_channel.h"

namespace Auris::CAN {
    class CanProviderChai : public ICanProvider {
      public:
        void Init() override;

        bool IsInited() override;

        std::shared_ptr<ICanChannel> CreateChannel(std::shared_ptr<ICanDevinfo> &devInfo, size_t channelIdx,
                                                           uint32_t baudrate) override;

        std::vector<std::shared_ptr<ICanDevinfo>> GetDeviceInfo() override;

      private:
        bool _inited = false;
    };
} // namespace Auris::CAN

#endif
