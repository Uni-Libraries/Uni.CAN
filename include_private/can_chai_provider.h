#pragma once

#if defined(_WIN32)

// Uni.CAN
#include "uni_can_devinfo.h"
#include "can_provider.h"
#include "can_chai_channel.h"

namespace Uni::CAN {
    class CanProviderChai : public ICanProvider {
    public:
        void Init() override;

        bool IsInited() override;

        ICanChannel *CreateChannel(uni_can_devinfo_t *devInfo, size_t channelIdx,
                                   uint32_t baudrate) override;

        std::vector<std::shared_ptr<uni_can_devinfo_t> > GetDeviceInfo() override;

    private:
        bool _inited = false;
    };
} // namespace Uni::CAN

#endif
