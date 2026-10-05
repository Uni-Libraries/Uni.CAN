#pragma once

// Uni.CAN
#include "can_channel_interface.h"

namespace Uni::CAN {
    /**
     * Make a channel of a backend written in C++ usable through the uni_can_channel_ functions
     * @param channel channel of the backend; the result owns it
     * @return the channel for the C functions, to be released with uni_can_channel_destroy();
     *         null when `channel` is null
     */
    void* CanChannelAdapt(ICanChannel* channel);
}
