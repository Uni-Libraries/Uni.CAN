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

#define UNI_CAN_DEVINFO_FIELDLEN (32U)



//
// Typedefs
//

typedef struct {

  char device_manufacturer [UNI_CAN_DEVINFO_FIELDLEN];
  char device_model        [UNI_CAN_DEVINFO_FIELDLEN];
  char device_sn           [UNI_CAN_DEVINFO_FIELDLEN];
  char device_type         [UNI_CAN_DEVINFO_FIELDLEN];
  uint32_t device_index;
  uint32_t device_chancnt;
} uni_can_devinfo_t;


#if defined(__cplusplus)
}
#endif
