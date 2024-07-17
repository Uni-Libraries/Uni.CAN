#if defined(__linux__)

// stdlib
#include <cstring>

// Uni.CAN
#include "can_socketcan_provider.h"
#include "can_socketcan_channel.h"
#include "uni_can_devinfo.h"
#include "common_sysfs.h"

using namespace std::string_literals;


namespace Uni::CAN {


    //
    // Public
    //

    void CanProviderSocketcan::Init() {
        // TODO
    }

    std::vector<std::shared_ptr<uni_can_devinfo_t>> CanProviderSocketcan::GetDeviceInfo() {
        std::vector<std::shared_ptr<uni_can_devinfo_t>> result;

        int can_idx = 0;
        while (true) {
            std::string can_name = "can"s + std::to_string(can_idx);
            if (!SysFs::IsClassExists("net", can_name)) {
                break;
            }

            auto can_ifidx = SysFs::GetClassProperty("net", can_name, "ifindex");
            if (can_ifidx >= 0) {
                auto* devinfo = new uni_can_devinfo_t; //TODO: can_name, can_ifidx
                strcpy(devinfo->device_manufacturer,"Linux");
                strcpy(devinfo->device_model, "SocketCAN");
                strcpy(devinfo->device_sn, can_name.c_str());
                devinfo->device_index = can_ifidx;
                result.push_back(std::shared_ptr<uni_can_devinfo_t>(devinfo));
            }
            can_idx++;
        }

        return result;
    }

    bool CanProviderSocketcan::IsInited() { return true; }

    ICanChannel* CanProviderSocketcan::CreateChannel(uni_can_devinfo_t* devInfo, size_t channelIdx, uint32_t baudrate) {
        ICanChannel* result = nullptr;
        if(devInfo != nullptr && baudrate != 0) {
            result = new CanChannelSocketcan(devInfo, channelIdx, baudrate);
        }
        return result;
    }
} // namespace Uni::CAN

#endif
