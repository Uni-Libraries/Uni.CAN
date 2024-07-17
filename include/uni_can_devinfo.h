#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

//
// Includes
//

//stdint
#include <stdint.h>


//
// Defines
//

#define UNI_CAN_DEVINFO_FIELDLEN (64U)


//
// Typedefs
//

typedef struct {
    char device_manufacturer[UNI_CAN_DEVINFO_FIELDLEN];
    char device_model[UNI_CAN_DEVINFO_FIELDLEN];
    char device_sn[UNI_CAN_DEVINFO_FIELDLEN];
    size_t device_index;
    uint32_t device_chancnt;
} uni_can_devinfo_t;


#if defined(__cplusplus)
}
#endif
