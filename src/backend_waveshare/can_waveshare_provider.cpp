#if defined(_WIN32)

// stdlib
#include <array>
#include <cstring>

// windows
#include <windows.h>

// waveshare
#include <ControlCAN.h>

// Uni.CAN
#include "backend_waveshare/can_waveshare_channel.h"
#include "backend_waveshare/can_waveshare_provider.h"



namespace Uni::CAN {
    void CanProviderWaveshare::Init() {
        if (!_inited) {
            _inited = true;
        }
    }

    std::vector<std::shared_ptr<uni_can_devinfo_t> > CanProviderWaveshare::GetDeviceInfo() {
        std::vector<std::shared_ptr<uni_can_devinfo_t> > result;

        if (!_inited) {
            return result;
        }

        std::array<VCI_BOARD_INFO, 64> board_info{};
        int device_count = VCI_FindUsbDevice2(board_info.data());
        for(size_t idx = 0; idx < device_count; idx++){
            auto* devinfo = new uni_can_devinfo_t{};
            strcpy(devinfo->device_manufacturer, "Waveshare");
            strcpy(devinfo->device_model, board_info[idx].str_hw_Type);
            strcpy(devinfo->device_provider, GetProviderName());
            strcpy(devinfo->device_sn, board_info[idx].str_Serial_Num);
            devinfo->device_chancnt = board_info[idx].can_Num;
            devinfo->device_index = idx;
            result.push_back(std::shared_ptr<uni_can_devinfo_t>(devinfo));
        }

        return result;
    }

    const char * CanProviderWaveshare::GetProviderName() const {
        return "wavesahre";
    }

    bool CanProviderWaveshare::IsInited() { return _inited; }

    ICanChannel *CanProviderWaveshare::CreateChannel(uni_can_devinfo_t *devInfo, size_t channelIdx, uint32_t baudrate) {
        ICanChannel *result = nullptr;
        if (devInfo != nullptr && channelIdx < 2 && baudrate != 0) {
            result = new CanChannelWaveshare(devInfo, channelIdx, baudrate);
        }
        return result;
    }
} // namespace Uni::CAN

#endif
