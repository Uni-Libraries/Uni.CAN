//
// Includes
//

// uni.can
#include "uni_can_j1939_msgdesc.h"

#include <string.h>


//
// Functions
//
bool uni_can_j1939_msgdesc_signal_get(const uni_can_message_t *msg, const uni_can_j1939_msgdesc_t *desc, size_t signal_id,
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

bool uni_can_j1939_msgdesc_signal_set(uni_can_message_t *msg, const uni_can_j1939_msgdesc_t *desc, size_t signal_id,
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
