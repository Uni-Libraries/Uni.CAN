#pragma once

// Uni.CAN
#include "can_provider.h"

namespace Uni::CAN {
    class CanProviderSocketcan : public ICanProvider {
    public:
        void Init() override;

        bool IsInited() override;

        ICanChannel *CreateChannel(uni_can_devinfo_t *devInfo, size_t channelIdx, uint32_t baudrate) override;

        std::vector<std::shared_ptr<uni_can_devinfo_t> > GetDeviceInfo() override;

        [[nodiscard]] const char* GetProviderName() const override;

    private:
        bool _inited = false;
    };
} // namespace Uni::CAN
