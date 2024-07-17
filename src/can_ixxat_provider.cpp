#if defined(_MSC_VER)

// VCI SDK
#include <vcisdk.h>

// Uni.CAN
#include "can_ixxat_channel.h"
#include "can_ixxat_provider.h"

namespace Uni::CAN {
    void CanProviderIxxat::Init() {
        // TODO
    }

    std::vector<std::shared_ptr<uni_can_devinfo_t> > CanProviderIxxat::GetDeviceInfo() {
        std::vector<std::shared_ptr<uni_can_devinfo_t> > result;

        IVciDeviceManager *dev_mgr = nullptr;
        IVciEnumDevice *dev_enum = nullptr;

        if (VciGetDeviceManager(&dev_mgr) == VCI_OK) {
            if (dev_mgr->EnumDevices(&dev_enum) == VCI_OK) {
                VCIDEVICEINFO binfo{};
                while (dev_enum->Next(1, &binfo, nullptr) == VCI_OK) {
                    auto *devinfo = new uni_can_devinfo_t; //TODO: can_name, can_ifidx
                    strcpy(devinfo->device_manufacturer, "HMS IXXAT");
                    strcpy(devinfo->device_model, binfo.Description);
                    strcpy(devinfo->device_sn, binfo.UniqueHardwareId.AsChar);
                    strcpy(devinfo->device_provider, GetProviderName());
                    devinfo->device_index = binfo.VciObjectId.AsInt64;
                    devinfo->device_chancnt = 1;
                    result.push_back(std::shared_ptr<uni_can_devinfo_t>(devinfo));
                }

                dev_enum->Release();
            }

            dev_mgr->Release();
        }

        return result;
    }

    const char * CanProviderIxxat::GetProviderName() const {
        return "ixxat";
    }

    bool CanProviderIxxat::IsInited() { return true; }

    ICanChannel *CanProviderIxxat::CreateChannel(uni_can_devinfo_t *devInfo, size_t channelIdx,
                                                 uint32_t baudrate) {
        ICanChannel *result = nullptr;
        if (devInfo != nullptr && baudrate != 0) {
            result = new CanChannelIxxat(devInfo, channelIdx, baudrate);
        }
        return result;
    }
} // namespace Uni::CAN

#endif
