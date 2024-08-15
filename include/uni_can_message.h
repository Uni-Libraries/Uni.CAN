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

typedef enum {
    UNI_CAN_MSG_FLAG_EXT_ID = 1 << 0,
    UNI_CAN_MSG_FLAG_TP     = 1 << 1,
} uni_can_message_flags_t;

typedef struct {
    union {
        uint64_t u64[UNI_CAN_MESSAGE_MAXLEN/8];
        uint32_t u32[UNI_CAN_MESSAGE_MAXLEN/4];
        uint16_t u16[UNI_CAN_MESSAGE_MAXLEN/2];
        uint8_t  u8 [UNI_CAN_MESSAGE_MAXLEN/1];
        void*    ptr;
    } data;

    uint32_t id;
    uni_can_message_flags_t flags;
    uint16_t len;
} uni_can_message_t;



//
// Functions
//

uni_can_message_t * uni_can_message_clone(const uni_can_message_t* msg);

uni_can_message_t* uni_can_message_create();

void uni_can_message_free(uni_can_message_t *msg);

int uni_can_message_to_string(const uni_can_message_t *msg, char *buf, size_t buf_size);

#if defined(__cplusplus)
}
#endif
