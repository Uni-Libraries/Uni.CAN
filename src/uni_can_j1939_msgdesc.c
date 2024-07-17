//
// Includes
//

// uni.can
#include "uni_can_j1939_msgdesc.h"

#include <string.h>


//
// Functions
//
bool uni_can_j1939_msgdesc_signal_get(uni_can_message_t *msg, uni_can_j1939_msgdesc_t *desc, size_t signal_id,
    float *value) {
    bool result = false;

    if(msg != NULL && desc != NULL && value != NULL) {
        size_t offset = 0U;
        size_t sig_idx = 0U;
        while(desc->signals[sig_idx] != NULL) {
            if(desc->signals[sig_idx]->id == signal_id) {
                if((offset % 8 == 0) && desc->signals[sig_idx]->length == 16) {
                    uint16_t value_u16 = 0;
                    memcpy(&value_u16, &msg->data[offset/8], sizeof(uint16_t));
                    *value = value_u16 * desc->signals[sig_idx]->scale + desc->signals[sig_idx]->offset;
                    result = true;
                }
                break;
            }
            offset += desc->signals[sig_idx]->length;
            sig_idx++;
        }
    }

    return result;
}

bool uni_can_j1939_msgdesc_signal_set(uni_can_message_t *msg, uni_can_j1939_msgdesc_t *desc, size_t signal_id,
                                      float value) {
    bool result = false;

    if(msg != NULL && desc != NULL) {
        size_t offset = 0U;
        size_t sig_idx = 0U;
        while(desc->signals[sig_idx] != NULL) {
            if(desc->signals[sig_idx]->id == signal_id) {
                if((offset % 8 == 0) && desc->signals[sig_idx]->length == 16) {
                    uint16_t value_u16 = value / desc->signals[sig_idx]->scale;
                    memcpy(&msg->data[offset/8], &value_u16, sizeof(uint16_t));
                    result = true;
                }
                break;
            }
            offset += desc->signals[sig_idx]->length;
            sig_idx++;
        }
    }

    return result;
}
