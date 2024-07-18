//
// Includes
//

// stdlib
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

bool uni_can_j1939_msg_pgn_request(uni_can_message_t *msg, size_t pgn_id, uint8_t addr_dst, uint8_t addr_src) {
    bool result = false;

    if(msg != NULL) {
        msg->len = 3;
        msg->id = uni_can_j1939_pgn_create(J1939_PRIORITY, UNI_CAN_J1939_PGN_REQUEST, addr_dst, addr_src);
        memcpy(msg->data, &pgn_id, msg->len);
        result = true;
    }

    return result;
}


bool uni_can_j1939_msg_signal_get(const uni_can_message_t *msg, const uni_can_j1939_msg_desc_t *desc, size_t signal_id,
    float *value) {
    bool result = false;

    if (msg != NULL && desc != NULL && value != NULL) {
        size_t offset = 0U;
        size_t sig_idx = 0U;
        while (desc->signal[sig_idx] != NULL) {
            uni_can_j1939_signal_t* signal = desc->signal[sig_idx];
            if (signal->id == signal_id) {
                if ((offset % 8) == 0 && (signal->length % 8) == 0) {
                    uint64_t value_u64 = 0;
                    memcpy(&value_u64, &msg->data[offset / 8], signal->length / 8);
                    *value = value_u64 * signal->scale + signal->offset;
                    result = true;
                }
                break;
            }
            offset += signal->length;
            sig_idx++;
        }
    }

    return result;
}

bool uni_can_j1939_msg_signal_set(uni_can_message_t *msg, const uni_can_j1939_msg_desc_t *desc, size_t signal_id,
                                      float value) {
    bool result = false;

    if (msg != NULL && desc != NULL) {
        size_t offset = 0U;
        size_t sig_idx = 0U;
        while (desc->signal[sig_idx] != NULL) {
            uni_can_j1939_signal_t* signal = desc->signal[sig_idx];
            if (signal->id == signal_id) {
                if ((offset % 8) == 0 && (signal->length % 8) == 0) {
                    uint64_t value_u64 = (value - signal->offset) / signal->scale;
                    memcpy(&msg->data[offset / 8], &value_u64, signal->length / 8);
                    result = true;
                }
                break;
            }
            offset += signal->length;
            sig_idx++;
        }
    }

    return result;
}
