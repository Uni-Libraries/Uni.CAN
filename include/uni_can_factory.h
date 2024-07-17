#pragma once
#include "../include_private/can_provider.h"

#if defined(__cplusplus)
extern "C" {
#endif

//
// Includes
//

// stdlib
#include <stddef.h>
#include <stdlib.h>

// uni.can
#include "uni_can_devinfo.h"


//
// Functions
//

size_t uni_can_factory_refresh();

bool uni_can_factory_get_info(uni_can_devinfo_t *info, size_t index);

void *uni_can_factory_create_channel(uni_can_devinfo_t *info, size_t channelidx, uint32_t baudrate);

#if defined(__cplusplus)
}
#endif
