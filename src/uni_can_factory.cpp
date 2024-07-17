//
// Includes
//

// stdlib
#include <string.h>

// Uni.CAN
#include "uni_can_factory.h"
#include "can_provider.h"
#include "can_chai_provider.h"
#include "can_ixxat_provider.h"
#include "can_socketcan_provider.h"


//
// Globals
//

static std::vector<std::shared_ptr<Uni::CAN::ICanProvider> > g_providers;
static std::vector<std::shared_ptr<uni_can_devinfo_t> > g_devices;


//
// Public
//

size_t uni_can_factory_refresh() {
    // providers
    if (g_providers.empty()) {
#if defined(_WIN32)
        g_providers.push_back(std::shared_ptr<Uni::CAN::ICanProvider>(new Uni::CAN::CanProviderChai()));
#endif
#if defined(_MSC_VER)
        g_providers.push_back(std::shared_ptr<Uni::CAN::ICanProvider>(new Uni::CAN::CanProviderIxxat()));
#endif
#if defined(__linux__)
        g_providers.push_back(std::make_shared<Uni::CAN::CanProviderSocketcan>());
#endif
    }

    // devices
    g_devices.clear();
    for (auto &provider: g_providers) {
        if (!provider->IsInited()) {
            provider->Init();
        }

        auto provider_info = provider->GetDeviceInfo();
        g_devices.insert(g_devices.end(), provider_info.begin(), provider_info.end());
    }

    return g_devices.size();
}


bool uni_can_factory_get_info(uni_can_devinfo_t *info, size_t index) {
    bool result = false;
    if (info != nullptr && index < g_devices.size()) {
        memcpy(info, g_devices[index].get(), sizeof(uni_can_devinfo_t));
        result = true;
    }

    return result;
}


void *uni_can_factory_create_channel(uni_can_devinfo_t *info, size_t channelidx, uint32_t baudrate) {
    if(!info) {
        return nullptr;
    }

    if(channelidx >= info->device_chancnt) {
        return nullptr;
    }

    for (auto &provider: g_providers) {
        if(strcmp(provider->GetProviderName(), info->device_provider) != 0) {
            continue;
        }

        auto result = provider->CreateChannel(info, channelidx, baudrate);
        if (result) {
            return result;
        }
    }
    return nullptr;
}
