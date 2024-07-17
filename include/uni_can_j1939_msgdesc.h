#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

//
// Includes
//

// stdlib
#include <stdbool.h>
#include <stddef.h>

// uni.can
#include "uni_can_message.h"
#include "uni_can_j1939_signal.h"


//
// Typedefs
//

typedef struct {
    /**
     * PGN Number
     */
    size_t pgn_number;

    /**
     * Message period in msec
     */
    size_t period;

    /**
     * Array of signals to parse
     */
    uni_can_j1939_signal_t **signal;
} uni_can_j1939_msgdesc_t;


//
// Functions
//

bool uni_can_j1939_msgdesc_signal_get(uni_can_message_t *msg, uni_can_j1939_msgdesc_t *desc, size_t signal_id,
                                      float *value);

bool uni_can_j1939_msgdesc_signal_set(uni_can_message_t *msg, uni_can_j1939_msgdesc_t *desc, size_t signal_id,
                                      float value);


#if defined(__cplusplus)
}
#endif
