#pragma once

// uni.can
#include "uni_can_message.h"

#if defined(__cplusplus)
extern "C" {
#endif

//
// Functions
//

bool uni_can_channel_init(void *channel);

bool uni_can_channel_open(void *channel);

bool uni_can_channel_close(void *channel);

bool uni_can_channel_destroy(void *channel);

uni_can_message_t* uni_can_channel_receive(void *channel);

bool uni_can_channel_transmit(void *channel, const uni_can_message_t *msg);

#if defined(__cplusplus)
}
#endif
