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
    UNI_CAN_J1939_SIGNAL_SIGNED   = 0,
    UNI_CAN_J1939_SIGNAL_UNSIGNED = 1,
    UNI_CAN_J1939_SIGNAL_FLOAT    = 2,
    UNI_CAN_J1939_SIGNAL_DOUBLE   = 3,

    UNI_CAN_J1939_SIGNAL_PGNNUM = 10,

    UNI_CAN_J1939_SIGNAL_SPECIAL_0 = 20,
    UNI_CAN_J1939_SIGNAL_SPECIAL_1 = 21,
    UNI_CAN_J1939_SIGNAL_SPECIAL_2 = 22,
    UNI_CAN_J1939_SIGNAL_SPECIAL_3 = 23,
    UNI_CAN_J1939_SIGNAL_SPECIAL_4 = 24,
    UNI_CAN_J1939_SIGNAL_SPECIAL_5 = 25,
    UNI_CAN_J1939_SIGNAL_SPECIAL_6 = 26,
    UNI_CAN_J1939_SIGNAL_SPECIAL_7 = 27,
    UNI_CAN_J1939_SIGNAL_SPECIAL_8 = 28,
    UNI_CAN_J1939_SIGNAL_SPECIAL_9 = 29,



} uni_can_j1939_signal_type_e;


typedef struct {
    size_t id;

    size_t length;

    float offset;

    float scale;

    float val_min;

    float val_max;

    const char *unit;

    uni_can_j1939_signal_type_e type;
} uni_can_j1939_signal_t;

#if defined(__cplusplus)
}
#endif
