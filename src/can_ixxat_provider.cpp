#if defined(_MSC_VER)

// VCI SDK
#include <vcisdk.h>

// auris.sdk --> private
#include "can_ixxat_channel.h"
#include "can_ixxat_devinfo.h"
#include "can_ixxat_provider.h"

namespace Auris::CAN {
    void CanProviderIxxat::Init() {
        // TODO
    }

    std::vector<std::shared_ptr<ICanDevinfo>> CanProviderIxxat::GetDeviceInfo() {
        std::vector<std::shared_ptr<ICanDevinfo>> result;

        IVciDeviceManager *dev_mgr = nullptr;
        IVciEnumDevice *dev_enum = nullptr;

        if (VciGetDeviceManager(&dev_mgr) == VCI_OK) {
            if (dev_mgr->EnumDevices(&dev_enum) == VCI_OK) {
                VCIDEVICEINFO dev_info{};
                while (dev_enum->Next(1, &dev_info, nullptr) == VCI_OK) {
                    result.push_back(std::shared_ptr<ICanDevinfo>(new CanDevinfoIxxat(dev_info)));
                }

                dev_enum->Release();
            }

            dev_mgr->Release();
        }

        return result;
    }

    bool CanProviderIxxat::IsInited() { return true; }

    std::shared_ptr<ICanChannel> CanProviderIxxat::CreateChannel(std::shared_ptr<ICanDevinfo> &devInfo, size_t channelIdx,
                                                                         uint32_t baudrate) {
        auto sp = std::dynamic_pointer_cast<CanDevinfoIxxat>(devInfo);
        if (!sp) {
            return nullptr;
        }

        return std::shared_ptr<ICanChannel>(new CanChannelIxxat(sp, channelIdx, baudrate));
    }
} // namespace Auris::CAN

#endif
