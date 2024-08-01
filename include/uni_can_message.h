#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

//
// Includes
//

// stdlib
#include <assert.h>
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
    union {
        uint64_t u64[UNI_CAN_MESSAGE_MAXLEN/8];
        uint32_t u32[UNI_CAN_MESSAGE_MAXLEN/4];
        uint16_t u16[UNI_CAN_MESSAGE_MAXLEN/2];
        uint8_t  u8 [UNI_CAN_MESSAGE_MAXLEN/1];
    } data;
    uint32_t id;
    uint8_t len;
} uni_can_message_t;

//
// Functions
//

int uni_can_message_to_string(const uni_can_message_t *msg, char *buf, size_t buf_size);


#if defined(__cplusplus)
}
#endif
