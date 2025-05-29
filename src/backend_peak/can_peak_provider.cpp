#if defined(_WIN32)

// stdlib
#include <cstring>

// windows
#include <windows.h>

// pcanbasic
#include <PCANBasic.h>

// Uni.CAN
#include "backend_peak/can_peak_channel.h"
#include "backend_peak/can_peak_provider.h"



namespace Uni::CAN {
    void CanProviderPeak::Init() {
        if (!_inited) {
            _inited = true;
        }
    }

    std::vector<std::shared_ptr<uni_can_devinfo_t> > CanProviderPeak::GetDeviceInfo() {
        std::vector<std::shared_ptr<uni_can_devinfo_t> > result;

        if (!_inited) {
            return result;
        }

        uint32_t chan_count{};
        if (CAN_GetValue(PCAN_NONEBUS, PCAN_ATTACHED_CHANNELS_COUNT, &chan_count, sizeof(chan_count)) != PCAN_ERROR_OK) {
            return {};
        }
        if (!chan_count) {
            return {};
        }

        std::vector<TPCANChannelInformation> chan_info(chan_count);
        if (CAN_GetValue(PCAN_NONEBUS, PCAN_ATTACHED_CHANNELS, chan_info.data(), chan_info.size() * sizeof(chan_info[0]))!= PCAN_ERROR_OK) {
            return {};
        }

        for (const auto& info: chan_info) {
            if (info.channel_condition & PCAN_CHANNEL_AVAILABLE) {
                auto* devinfo = new uni_can_devinfo_t{};
                strcpy(devinfo->device_manufacturer, "PEAK-System");
                strcpy(devinfo->device_model, info.device_name);
                strcpy(devinfo->device_provider, GetProviderName());
                devinfo->device_chancnt = 1;
                devinfo->device_index = info.channel_handle;
                char chnum[8] {};
                sprintf(chnum, "0x%X", info.device_id);
                strcpy(devinfo->device_sn, chnum);
                result.push_back(std::shared_ptr<uni_can_devinfo_t>(devinfo));
            }
        }

        return result;
    }

    const char * CanProviderPeak::GetProviderName() const {
        return "pcan";
    }

    bool CanProviderPeak::IsInited() { return _inited; }

    ICanChannel *CanProviderPeak::CreateChannel(uni_can_devinfo_t *devInfo, size_t channelIdx, uint32_t baudrate) {
        ICanChannel *result = nullptr;
        if (devInfo != nullptr && baudrate != 0) {
            result = new CanChannelPeak(devInfo, baudrate);
        }
        return result;
    }
} // namespace Uni::CAN

#endif
