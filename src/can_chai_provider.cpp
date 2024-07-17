#if defined(_WIN32)

// CHAI SDK
#include <chai.h>

// Uni.CAN
#include "can_chai_channel.h"
#include "can_chai_provider.h"


namespace Uni::CAN {
    void CanProviderChai::Init() {
        if (!_inited) {
            CiInit();
            _inited = true;
        }
    }

    std::vector<std::shared_ptr<uni_can_devinfo_t> > CanProviderChai::GetDeviceInfo() {
        std::vector<std::shared_ptr<uni_can_devinfo_t> > result;

        if (!_inited) {
            return result;
        }

        canboard_t binfo{};
        for (uint8_t i = 0; i < CI_BRD_NUMS; i++) {
            binfo.brdnum = i;
            if (CiBoardInfo(&binfo) == 0) {
                auto *devinfo = new uni_can_devinfo_t;
                strcpy(devinfo->device_manufacturer, binfo.manufact);
                strcpy(devinfo->device_model, binfo.name);
                devinfo->device_index = binfo.brdnum;
                devinfo->device_chancnt = 1;
                result.push_back(std::shared_ptr<uni_can_devinfo_t>(devinfo));
            }
        }
        return result;
    }

    bool CanProviderChai::IsInited() { return _inited; }

    ICanChannel *CanProviderChai::CreateChannel(uni_can_devinfo_t *devInfo, size_t channelIdx, uint32_t baudrate) {
        ICanChannel *result = nullptr;
        if (devInfo != nullptr && baudrate != 0) {
            result = new CanChannelChai(devInfo, channelIdx, baudrate);
        }
        return result;
    }
} // namespace Uni::CAN

#endif
