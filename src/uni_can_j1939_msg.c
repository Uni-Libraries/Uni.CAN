//
// Includes
//

// stdlib
#include <math.h>
#include <string.h>

// uni.can
#include "uni_can_j1939_msg.h"

#include "uni_can_j1939_const.h"
#include "uni_can_j1939_pgn.h"


//
// Defines
//

#define J1939_PRIORITY 6U


//
// Functions
//

bool uni_can_j1939_msg_pgn_request(uni_can_message_t *msg, uint32_t pgn_id, uint8_t addr_dst, uint8_t addr_src) {
    bool result = false;

    if (msg != NULL) {
        msg->len = 3;
        msg->id = uni_can_j1939_pgn_create(J1939_PRIORITY, UNI_CAN_J1939_PGN_REQUEST, addr_dst, addr_src);
        msg->data.u32[0] = pgn_id;
        result = true;
    }

    return result;
}


bool uni_can_j1939_msg_signal_get(const uni_can_message_t *msg, const uni_can_j1939_msg_desc_t *desc, size_t signal_id,
                                  uni_can_j1939_signal_value_t *value) {
    bool result = false;

    if (msg != NULL && desc != NULL && value != NULL) {
        size_t offset = 0U;
        size_t sig_idx = 0U;
        while (desc->signal[sig_idx] != NULL) {
            uni_can_j1939_signal_t *signal = desc->signal[sig_idx];
            if (signal->id == signal_id) {
                uint64_t val_max = (1ULL << signal->length) - 1;
                uint64_t val = (msg->data.u64[0] >> offset) & val_max;
                if (signal->type == UNI_CAN_J1939_SIGNAL_SLOT) {
                    if (val == val_max) {
                        value->slot = nan("");
                    }
                    else {
                        value->slot = val * signal->scale + signal->offset;
                    }
                }
                else {
                    value->raw_uint64 = val;
                }
                result = true;
            }
            offset += signal->length;
            sig_idx++;
        }
    }

    return result;
}

bool uni_can_j1939_msg_signal_set(uni_can_message_t *msg, const uni_can_j1939_msg_desc_t *desc, size_t signal_id,
                                  const uni_can_j1939_signal_value_t *value) {

    bool result = false;

    if (msg != NULL && desc != NULL && value != NULL) {
        size_t offset = 0U;
        size_t sig_idx = 0U;
        while (desc->signal[sig_idx] != NULL) {
            uni_can_j1939_signal_t *signal = desc->signal[sig_idx];
            if (signal->id == signal_id) {
                uint64_t val_max = (1ULL << signal->length) - 1;
                if (signal->type == UNI_CAN_J1939_SIGNAL_SLOT) {
                    uint64_t val = (uint64_t)((value->slot - signal->offset) / signal->scale) & val_max;
                    msg->data.u64[0] |= val << offset;
                }
                else {
                    uint64_t val = value->raw_uint64 & val_max;
                    msg->data.u64[0] |= val << offset;
                }
                result = true;
            }
            offset += signal->length;
            sig_idx++;
        }
    }
    return result;
}
