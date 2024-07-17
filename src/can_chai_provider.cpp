#if defined(_WIN32)

// CHAI SDK
#include <chai.h>

// auris.can --> private
#include "can_chai_channel.h"
#include "can_chai_devinfo.h"
#include "can_chai_provider.h"


namespace Auris::CAN {
    void CanProviderChai::Init() {
        if (!_inited) {
            CiInit();
            _inited = true;
        }
    }

    std::vector<std::shared_ptr<ICanDevinfo>> CanProviderChai::GetDeviceInfo() {
        std::vector<std::shared_ptr<ICanDevinfo>> result;
        if (!_inited) {
            return result;
        }

        canboard_t binfo{};
        for (uint8_t i = 0; i < CI_BRD_NUMS; i++) {
            binfo.brdnum = i;
            if (CiBoardInfo(&binfo) == 0) {
                result.push_back(std::shared_ptr<ICanDevinfo>(new CanDevinfoChai(binfo)));
            }
        }
        return result;
    }

    bool CanProviderChai::IsInited() { return _inited; }

    std::shared_ptr<ICanChannel> CanProviderChai::CreateChannel(std::shared_ptr<ICanDevinfo> &devInfo, size_t channelIdx,
                                                                        uint32_t baudrate) {
        auto sp = std::dynamic_pointer_cast<CanDevinfoChai>(devInfo);
        if (!sp) {
            return nullptr;
        }

        return std::shared_ptr<ICanChannel>(new CanChannelChai(sp, channelIdx, baudrate));
    }
} // namespace Auris::CAN

#endif
