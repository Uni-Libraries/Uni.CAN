#pragma once

// ProtoPlexer protocol (C API)

#if defined(__cplusplus)
extern "C" {
#endif

// stdlib
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// uni.can
#include "uni_can_message.h"

//
// Constants (from reference protoplexer.h)
//

#define UNI_CAN_PROTOPLEXER_PRIORITY_TOP     0x00
#define UNI_CAN_PROTOPLEXER_PRIORITY_DEFAULT 0x07
#define UNI_CAN_PROTOPLEXER_PRIORITY_MINIMAL 0x0F
#define UNI_CAN_PROTOPLEXER_PRIORITY_MASK    0x0F

#define UNI_CAN_PROTOPLEXER_ADDRESS_MAX  0x0FFF
#define UNI_CAN_PROTOPLEXER_ADDRESS_MIN  0x0001
#define UNI_CAN_PROTOPLEXER_ADDRESS_MASK 0x0FFF

#define UNI_CAN_PROTOPLEXER_CHUNK_STARTING_LENGTH_MIN 6

#define UNI_CAN_PROTOPLEXER_HEADER_ID_OFFSET     0
#define UNI_CAN_PROTOPLEXER_HEADER_LENGTH_OFFSET 2
#define UNI_CAN_PROTOPLEXER_HEADER_CRC_OFFSET    4
#define UNI_CAN_PROTOPLEXER_HEADER_DATA_OFFSET   6
#define UNI_CAN_PROTOPLEXER_HEADER_SIZE          6

#define UNI_CAN_PROTOPLEXER_ERROR_ID 0x0000


//
// Types
//

typedef enum {
    UNI_CAN_PROTOPLEXER_OK = 0,
    UNI_CAN_PROTOPLEXER_FAILURE,
    UNI_CAN_PROTOPLEXER_WRONG_ADDRESS,
    UNI_CAN_PROTOPLEXER_WRONG_PRIORITY,
    UNI_CAN_PROTOPLEXER_ADDING_CHUNK_CANT_ALLOCATE_NEW_MSG,
    UNI_CAN_PROTOPLEXER_ADDING_CHUNK_FIRST_AND_TOO_SHORT,
    UNI_CAN_PROTOPLEXER_ADDING_CHUNK_UNRELATED_ADDRESS_TO,
    UNI_CAN_PROTOPLEXER_ADDING_CHUNK_HEADER_DATA_OVERFLOW,
    UNI_CAN_PROTOPLEXER_ADDING_CHUNK_CANT_ALLOCATE_DATA,
    UNI_CAN_PROTOPLEXER_ADDING_CHUNK_RECEIVED_ERROR_ID,
    UNI_CAN_PROTOPLEXER_ADDING_CHUNK_DATA_WITHOUT_HEADER,
    UNI_CAN_PROTOPLEXER_ADDING_CHUNK_DATA_OVERFLOW,
    UNI_CAN_PROTOPLEXER_ADDING_CHUNK_CANT_SAVE_DATA,
    UNI_CAN_PROTOPLEXER_ADDING_CHUNK_CANT_SAVE_MESSAGE,
    UNI_CAN_PROTOPLEXER_ADDING_CHUNK_INVALID_CHUNK,
    UNI_CAN_PROTOPLEXER_ADDING_CHUNK_WRONG_CRC,
    UNI_CAN_PROTOPLEXER_SEND_MSG_HW_CANT_SEND_HEADER,
    UNI_CAN_PROTOPLEXER_SEND_MSG_CANT_ADD_MESSAGE_CHUNKS,
    UNI_CAN_PROTOPLEXER_SEND_MSG_CANT_WRITE_HEADER,
} uni_can_protoplexer_result_t;


typedef struct {
    uint16_t message_id;
    uint16_t address_from;
    uint16_t address_to;
    uint8_t priority_inverted;

    uint16_t length;
    uint8_t *data; // owned by this struct
} uni_can_protoplexer_msg_t;


// Opaque RX state is stored inside ctx->rx as a private pointer.
typedef struct {
    uint16_t own_address;
    uint16_t max_chunk_length;
    bool monitoring;

    uint16_t max_channels;
    void *rx;
} uni_can_protoplexer_ctx_t;

typedef struct {
    uint16_t own_address;
    uint16_t max_chunk_length; // for CAN classic should be 8
    uint16_t max_channels;     // max concurrent in-progress (from,to) reassemblies
    bool monitoring;
} uni_can_protoplexer_config_t;


//
// Utilities
//

const char *uni_can_protoplexer_result_to_string(uni_can_protoplexer_result_t value);

uint16_t uni_can_protoplexer_crc16(const uint8_t *buf, uint32_t len);

uint32_t uni_can_protoplexer_canid_create(uint16_t address_from, uint16_t address_to, uint8_t priority_inverted, bool is_first);
uint16_t uni_can_protoplexer_canid_get_from(uint32_t can_id);
uint16_t uni_can_protoplexer_canid_get_to(uint32_t can_id);
uint8_t uni_can_protoplexer_canid_get_priority_inverted(uint32_t can_id);
bool uni_can_protoplexer_canid_get_is_first(uint32_t can_id);


//
// Message
//

uni_can_protoplexer_msg_t *uni_can_protoplexer_msg_create(uint16_t message_id, uint16_t address_from, uint16_t address_to,
                                                          uint8_t priority_inverted, const uint8_t *data, uint16_t length);
void uni_can_protoplexer_msg_free(uni_can_protoplexer_msg_t *msg);


//
// Protocol core
//

bool uni_can_protoplexer_init(uni_can_protoplexer_ctx_t *ctx, const uni_can_protoplexer_config_t *cfg);
void uni_can_protoplexer_deinit(uni_can_protoplexer_ctx_t *ctx);

// Fragment a Protoplexer message into CAN chunks (each chunk is an uni_can_message_t).
// Returned array must be freed with uni_can_protoplexer_free_chunks().
uni_can_protoplexer_result_t uni_can_protoplexer_build_chunks(const uni_can_protoplexer_msg_t *msg,
                                                              uint16_t max_chunk_length,
                                                              uni_can_message_t **out_chunks,
                                                              size_t *out_count);
void uni_can_protoplexer_free_chunks(uni_can_message_t *chunks);

// Add a received CAN chunk. If a full Protoplexer message is completed successfully, *out_msg will be set
// to a newly allocated uni_can_protoplexer_msg_t (must be freed with uni_can_protoplexer_msg_free()).
uni_can_protoplexer_result_t uni_can_protoplexer_add_chunk(uni_can_protoplexer_ctx_t *ctx,
                                                          const uni_can_message_t *chunk,
                                                          uni_can_protoplexer_msg_t **out_msg);


#if defined(__cplusplus)
}
#endif
