#if defined(_WIN32)

// stdlib
#include <cstring>

// windows
#include <windows.h>

// pcanbasic
#include <vxlapi.h>

// Uni.CAN
#include "backend_vector/can_vector_channel.h"
#include "backend_vector/can_vector_provider.h"



namespace Uni::CAN {
    CanProviderVector::~CanProviderVector() {
        if (_inited) {
            xlCloseDriver();
        }
    }

    void CanProviderVector::Init() {
        if (!_inited) {
            _inited = xlOpenDriver() == XL_SUCCESS;
        }
    }

    std::vector<std::shared_ptr<uni_can_devinfo_t> > CanProviderVector::GetDeviceInfo() {
        std::vector<std::shared_ptr<uni_can_devinfo_t> > result;

        if (!_inited) {
            return result;
        }

        XLdriverConfig drvconfig{};
        if (xlGetDriverConfig(&drvconfig) != XL_SUCCESS) {
            return result;
        }

        for (size_t idx = 0; idx < drvconfig.channelCount; idx++) {
            //skip device without can support
            if((drvconfig.channel[idx].channelBusCapabilities & XL_BUS_ACTIVE_CAP_CAN) == 0)
            {
                continue;
            }

            //skip virtual devices
            if(drvconfig.channel[idx].hwType == XL_HWTYPE_VIRTUAL)
            {
                continue;
            }

            auto* devinfo = new uni_can_devinfo_t{};
            strcpy(devinfo->device_manufacturer, "Vector");
            strcpy(devinfo->device_model, drvconfig.channel[idx].name);
            strcpy(devinfo->device_provider, GetProviderName());
            devinfo->device_chancnt = 1;
            devinfo->device_index = drvconfig.channel[idx].channelMask;

            char chnum[16] {};
            sprintf(chnum, "%d", drvconfig.channel[idx].serialNumber);
            strcpy(devinfo->device_sn, chnum);

            result.push_back(std::shared_ptr<uni_can_devinfo_t>(devinfo));
        }

        return result;
    }

    const char * CanProviderVector::GetProviderName() const {
        return "vector";
    }

    bool CanProviderVector::IsInited() { return _inited; }

    ICanChannel *CanProviderVector::CreateChannel(uni_can_devinfo_t *devInfo, size_t channelIdx, uint32_t baudrate) {
        ICanChannel *result = nullptr;
        if (devInfo != nullptr && baudrate != 0) {
            result = new CanChannelVector(devInfo, baudrate);
        }
        return result;
    }
} // namespace Uni::CAN

#endif
