//
// Includes
//

// stdlib
#include <cstring>

// CHAI SDK
#include <chai.h>

// Uni.CAN
#include "can_marathon_channel.h"
#include "can_marathon_provider.h"


namespace Uni::CAN {
    void CanProviderMarathon::Init() {
        if (!_inited) {
            CiInit();
            _inited = true;
        }
    }

    std::vector<std::shared_ptr<uni_can_devinfo_t> > CanProviderMarathon::GetDeviceInfo() {
        std::vector<std::shared_ptr<uni_can_devinfo_t> > result;

        if (!_inited) {
            return result;
        }

        for (uint8_t i = 0; i < CI_CHAN_NUMS; i++) {
            if (CiOpen(i, 0) == 0) {
                chipstat_t stat{};
                if (CiChipStat(i, &stat) == 0) {

                    canboard_t binfo{};
                    binfo.brdnum = stat.brdnum;
                    if (CiBoardInfo(&binfo) == 0) {
                        auto* devinfo = new uni_can_devinfo_t{};
                        strcpy(devinfo->device_manufacturer, binfo.manufact);
                        if (!strlen(devinfo->device_manufacturer))
                        {
                            strcpy(devinfo->device_manufacturer, "Marathon");
                        }
                        strcpy(devinfo->device_model, binfo.name);
                        strcpy(devinfo->device_provider, GetProviderName());
                        devinfo->device_index = i;
                        devinfo->device_chancnt = 1;
                        CiBoardGetSerial(stat.brdnum, devinfo->device_sn, sizeof(devinfo->device_sn));
                        strcat(devinfo->device_sn, " CH");
                        char chnum[8] {};
                        sprintf(chnum, "%d", i);
                        strcat(devinfo->device_sn, chnum);

                        result.push_back(std::shared_ptr<uni_can_devinfo_t>(devinfo));
                    }
                }
                CiClose(i);
            }
        }

        return result;
    }

    const char * CanProviderMarathon::GetProviderName() const {
        return "marathon";
    }

    bool CanProviderMarathon::IsInited() { return _inited; }

    ICanChannel *CanProviderMarathon::CreateChannel(uni_can_devinfo_t *devInfo, size_t channelIdx, uint32_t baudrate) {
        ICanChannel *result = nullptr;
        if (devInfo != nullptr && baudrate != 0) {
            result = new CanChannelMarathon(devInfo, devInfo->device_index, baudrate);
        }
        return result;
    }
} // namespace Uni::CAN
