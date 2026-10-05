#pragma once

// ProtoPlexer protocol with queues (C API)
//
// The protocol with an outbox for the chunks that wait to be sent and an inbox for the messages
// that were received: the "queued plexer" of the reference implementation. All memory comes
// from the application, the heap is not used.

#if defined(__cplusplus)
extern "C" {
#endif

// stdlib
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// uni.can
#include "uni_can_message.h"
#include "uni_can_protoplexer.h"


//
// Types
//

// What became of a chunk that was handed to the network.
typedef enum {
    UNI_CAN_PROTOPLEXER_HW_OK = 0,
    // not sent, try the same chunk again later
    UNI_CAN_PROTOPLEXER_HW_ERROR_RESEND_REQUIRED,
    // not sent and never will be: the chunk is dropped
    UNI_CAN_PROTOPLEXER_HW_ERROR_NON_RECOVERABLE,
} uni_can_protoplexer_hw_result_t;

// Hands a chunk to the network.
typedef uni_can_protoplexer_hw_result_t (*uni_can_protoplexer_send_chunk_f)(void *cookie, const uni_can_message_t *chunk);

// Takes a received chunk from the network. Returns false when there is none.
typedef bool (*uni_can_protoplexer_receive_chunk_f)(void *cookie, uni_can_message_t *chunk);

// Gets a received message. The message and its data are valid during the call only.
typedef void (*uni_can_protoplexer_rx_event_f)(void *cookie, const uni_can_protoplexer_msg_t *msg);

// Memory of a queue. UNI_CAN_PROTOPLEXER_QUEUE_STORAGE_DEFINITION() defines one.
typedef struct {
    uni_can_protoplexer_rx_buffer_t *rx_buffers; // max_channels elements
    uint8_t *rx_payload;                         // max_channels * max_payload bytes
    uint16_t max_channels;                       // max concurrent in-progress (from,to) reassemblies
    uint16_t max_payload;                        // longest message data, sent or received

    uni_can_protoplexer_msg_t *inbox;            // inbox_size elements
    uint8_t *inbox_payload;                      // inbox_size * max_payload bytes
    size_t inbox_size;                           // completed messages that can wait for poll_new

    uni_can_message_t *outbox;                   // outbox_size elements
    size_t outbox_size;                          // chunks that can wait for poll_tx
} uni_can_protoplexer_queue_storage_t;

#define UNI_CAN_PROTOPLEXER_QUEUE_STORAGE_DEFINITION(name, channels, payload, inbox_count, outbox_count)              \
    static uni_can_protoplexer_rx_buffer_t name##_rx_buffers[(channels)];                                             \
    static uint8_t name##_rx_payload[(channels) * (payload)];                                                         \
    static uni_can_protoplexer_msg_t name##_inbox[(inbox_count)];                                                     \
    static uint8_t name##_inbox_payload[(inbox_count) * (payload)];                                                   \
    static uni_can_message_t name##_outbox[(outbox_count)];                                                           \
    static const uni_can_protoplexer_queue_storage_t name = {                                                         \
        .rx_buffers = name##_rx_buffers,                                                                              \
        .rx_payload = name##_rx_payload,                                                                              \
        .max_channels = (channels),                                                                                   \
        .max_payload = (payload),                                                                                     \
        .inbox = name##_inbox,                                                                                        \
        .inbox_payload = name##_inbox_payload,                                                                        \
        .inbox_size = (inbox_count),                                                                                  \
        .outbox = name##_outbox,                                                                                      \
        .outbox_size = (outbox_count),                                                                                \
    }

typedef struct {
    uint16_t own_address;
    uint16_t max_chunk_length; // for CAN classic should be 8
    bool monitoring;

    // Times in a row the network may ask for the same chunk again before poll_tx reports
    // UNI_CAN_PROTOPLEXER_HW_SEND_TOO_MANY_ATTEMPTS. The chunk stays in the outbox.
    uint32_t send_attempts;

    // Has to stay valid, with the memory it points to, while the queue is in use.
    const uni_can_protoplexer_queue_storage_t *storage;

    uni_can_protoplexer_send_chunk_f send_chunk;
    uni_can_protoplexer_receive_chunk_f receive_chunk;
    uni_can_protoplexer_rx_event_f rx_event;
    void *cookie;
} uni_can_protoplexer_queue_config_t;

typedef struct {
    uni_can_protoplexer_queue_config_t config;
    uni_can_protoplexer_ctx_t protocol;

    size_t inbox_head;
    size_t inbox_count;

    size_t outbox_head;
    size_t outbox_count;

    uint32_t send_attempts;

    bool initialized;
} uni_can_protoplexer_queue_t;


//
// Functions
//

bool uni_can_protoplexer_queue_init(uni_can_protoplexer_queue_t *queue, const uni_can_protoplexer_queue_config_t *cfg);

// Queue a message for transmission. Either all of its chunks are queued or none.
uni_can_protoplexer_result_t uni_can_protoplexer_queue_send(uni_can_protoplexer_queue_t *queue, uint16_t message_id,
                                                           uint16_t address_from, uint16_t address_to,
                                                           uint8_t priority_inverted, const uint8_t *data,
                                                           uint16_t length);

// Process a received chunk. A message that it completes goes to the inbox.
uni_can_protoplexer_result_t uni_can_protoplexer_queue_add_chunk(uni_can_protoplexer_queue_t *queue,
                                                                const uni_can_message_t *chunk);

// Take one chunk from the network and process it.
// Returns UNI_CAN_PROTOPLEXER_POLLING_RX_HW_RX_EMPTY when the network has none.
uni_can_protoplexer_result_t uni_can_protoplexer_queue_poll_rx(uni_can_protoplexer_queue_t *queue);

// Hand the oldest chunk of the outbox to the network.
// Returns UNI_CAN_PROTOPLEXER_POLLING_TX_BUFFER_EMPTY when the outbox is empty.
uni_can_protoplexer_result_t uni_can_protoplexer_queue_poll_tx(uni_can_protoplexer_queue_t *queue);

// Pass the oldest message of the inbox to rx_event.
// Returns UNI_CAN_PROTOPLEXER_POLLING_NEW_NO_NEW_MESSAGES when the inbox is empty.
uni_can_protoplexer_result_t uni_can_protoplexer_queue_poll_new(uni_can_protoplexer_queue_t *queue);


#if defined(__cplusplus)
}
#endif
