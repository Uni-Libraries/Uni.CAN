//
// Includes
//

// stdlib
#include <stdlib.h>

// uni.can
#include "uni_can_message.h"

#include <string.h>


//
// Functions
//

uni_can_message_t * uni_can_message_clone(const uni_can_message_t *msg) {
    uni_can_message_t* result = uni_can_message_create();
    if(result != NULL) {
        memcpy(result, msg, sizeof(uni_can_message_t));

        //TP
        if(result->flags & UNI_CAN_MSG_FLAG_TP) {
            result->data.ptr = malloc(result->len);
            if(result->data.ptr != NULL) {
                memcpy(result->data.ptr, msg->data.ptr, result->len);
            }
        }
    }
    return result;
}


uni_can_message_t * uni_can_message_create() {
    return calloc(1, sizeof(uni_can_message_t));
}


void uni_can_message_free(uni_can_message_t *msg) {
    if (msg != NULL && (msg->flags & UNI_CAN_MSG_FLAG_TP)) {
        free(msg->data.ptr);
    }
    free(msg);
}


int uni_can_message_to_string(const uni_can_message_t *msg, char *buf, size_t buf_size) {
    return snprintf(buf, buf_size, "id: %d, dlc: %d, data: TODO", msg->id, msg->len);
}
