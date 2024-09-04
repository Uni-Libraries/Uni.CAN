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
    UNI_CAN_MSG_FLAG_STD_ID = 1 << 0,
    UNI_CAN_MSG_FLAG_EXT_ID = 1 << 1,
    UNI_CAN_MSG_FLAG_TP     = 1 << 2,
} uni_can_message_flags_t;

typedef union {
    uint64_t u64[UNI_CAN_MESSAGE_MAXLEN/8];
    uint32_t u32[UNI_CAN_MESSAGE_MAXLEN/4];
    uint16_t u16[UNI_CAN_MESSAGE_MAXLEN/2];
    uint8_t  u8 [UNI_CAN_MESSAGE_MAXLEN/1];
    uint8_t* ptr;
} uni_can_message_data_t;

typedef struct {
    uni_can_message_data_t data;
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


//
// CPP
//
#if defined(__cplusplus)
#include <type_traits>

constexpr uni_can_message_flags_t operator|(uni_can_message_flags_t lhs, uni_can_message_flags_t rhs) {
    return static_cast<uni_can_message_flags_t>(static_cast<std::underlying_type<uni_can_message_flags_t>::type>(lhs) |
                                     static_cast<std::underlying_type<uni_can_message_flags_t>::type>(rhs));
}
#endif
