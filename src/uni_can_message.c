//
// Includes
//

// uni.can
#include "uni_can_message.h"


//
// Functions
//

int uni_can_message_to_string(const uni_can_message_t *msg, char *buf, size_t buf_size) {
    return snprintf(buf, buf_size, "id: %d, dlc: %d, data: TODO", msg->id, msg->len);
}
