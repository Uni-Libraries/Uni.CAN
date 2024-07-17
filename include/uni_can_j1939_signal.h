#pragma once

//
// Includes
//

#if defined(__cplusplus)
extern "C" {
#endif


// stdlib
#include <stddef.h>

//
// Typedefs
//

typedef enum {
      UNI_CAN_J1939_SIGNAL_SIGNED,
      UNI_CAN_J1939_SIGNAL_UNSIGNED,
      UNI_CAN_J1939_SIGNAL_FLOAT,
      UNI_CAN_J1939_SIGNAL_DOUBLE,
} uni_can_j1939_signal_type_e;


typedef struct {
      size_t id;

      size_t length;

      float offset;

      float scale;

      float val_min;

      float val_max;

      const char* unit;

      uni_can_j1939_signal_type_e type;
} uni_can_j1939_signal_t;

#if defined(__cplusplus)
}
#endif