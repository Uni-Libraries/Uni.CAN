#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

//
// Includes
//

// stdlib
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


//
// Defines
//

#define UNI_CAN_MESSAGE_MAXLEN (8U)


//
// Typedef
//

typedef struct {
    uint32_t id;
    uint8_t len;
    uint8_t data[UNI_CAN_MESSAGE_MAXLEN];
} uni_can_message_t;


//
// Functions
//

int uni_can_message_to_string(const uni_can_message_t *msg, char *buf, size_t buf_size);


#if defined(__cplusplus)
}
#endif
