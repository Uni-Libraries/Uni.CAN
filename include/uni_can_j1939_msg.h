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
} uni_can_j1939_msg_desc_t;


//
// Functions
//


bool uni_can_j1939_msg_pgn_request(uni_can_message_t *msg, uint32_t pgn_id, uint8_t addr_dst, uint8_t addr_src);

bool uni_can_j1939_msg_signal_get(const uni_can_message_t *msg, const uni_can_j1939_msg_desc_t *desc,
                                  size_t signal_id, uni_can_j1939_signal_value_t *value);

bool uni_can_j1939_msg_signal_set(uni_can_message_t *msg, const uni_can_j1939_msg_desc_t *desc, size_t signal_id,
                                  const uni_can_j1939_signal_value_t* value);


#if defined(__cplusplus)
}
#endif
