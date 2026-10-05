//
// ProtoPlexer protocol with queues (C implementation)
//

// stdlib
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// uni.can
#include "uni_can_protoplexer_queue.h"


//
// Internal
//

static bool uni_can_protoplexer_queue_inbox_push(uni_can_protoplexer_queue_t *queue, const uni_can_protoplexer_msg_t *msg) {
    const uni_can_protoplexer_queue_storage_t *storage = queue->config.storage;
    if (queue->inbox_count >= storage->inbox_size) {
        return false;
    }

    // every element of the inbox has its own part of the payload memory
    const size_t tail = (queue->inbox_head + queue->inbox_count) % storage->inbox_size;
    uni_can_protoplexer_msg_t *slot = &storage->inbox[tail];
    *slot = *msg;
    slot->data = NULL;
    if (msg->length > 0) {
        slot->data = &storage->inbox_payload[tail * storage->max_payload];
        memcpy(slot->data, msg->data, msg->length);
    }

    queue->inbox_count++;
    return true;
}

static void uni_can_protoplexer_queue_outbox_pop(uni_can_protoplexer_queue_t *queue) {
    queue->outbox_head = (queue->outbox_head + 1U) % queue->config.storage->outbox_size;
    queue->outbox_count--;
}


//
// Public API
//

bool uni_can_protoplexer_queue_init(uni_can_protoplexer_queue_t *queue, const uni_can_protoplexer_queue_config_t *cfg) {
    if (!queue || !cfg || !cfg->storage) {
        return false;
    }

    const uni_can_protoplexer_queue_storage_t *storage = cfg->storage;
    if (!storage->rx_buffers || !storage->inbox || storage->inbox_size == 0 || !storage->outbox || storage->outbox_size == 0) {
        return false;
    }
    if (storage->max_payload != 0 && (!storage->rx_payload || !storage->inbox_payload)) {
        return false;
    }

    memset(queue, 0, sizeof(*queue));
    queue->config = *cfg;

    const uni_can_protoplexer_config_t protocol_cfg = {
        .own_address = cfg->own_address,
        .max_chunk_length = cfg->max_chunk_length,
        .max_channels = storage->max_channels,
        .monitoring = cfg->monitoring,
        .max_payload = storage->max_payload,
        .rx_buffers = storage->rx_buffers,
        .rx_payload = storage->rx_payload,
    };
    if (!uni_can_protoplexer_init_static(&queue->protocol, &protocol_cfg)) {
        memset(queue, 0, sizeof(*queue));
        return false;
    }

    queue->initialized = true;
    return true;
}

uni_can_protoplexer_result_t uni_can_protoplexer_queue_send(uni_can_protoplexer_queue_t *queue, uint16_t message_id,
                                                           uint16_t address_from, uint16_t address_to,
                                                           uint8_t priority_inverted, const uint8_t *data,
                                                           uint16_t length) {
    if (!queue || !queue->initialized) {
        return UNI_CAN_PROTOPLEXER_FAILURE;
    }

    const uni_can_protoplexer_queue_storage_t *storage = queue->config.storage;
    if (length > storage->max_payload || (length > 0 && !data)) {
        return UNI_CAN_PROTOPLEXER_SEND_MSG_INVALID_SIZE;
    }

    // reject the whole message if the outbox can't take all of its chunks
    const size_t chunks_count = uni_can_protoplexer_chunks_count(length, queue->protocol.max_chunk_length);
    if (chunks_count == 0) {
        return UNI_CAN_PROTOPLEXER_SEND_MSG_HW_CANT_SEND_HEADER;
    }
    if (queue->outbox_count + chunks_count > storage->outbox_size) {
        return UNI_CAN_PROTOPLEXER_SEND_MSG_CANT_ADD_MESSAGE_CHUNKS;
    }

    // the chunks are built from the data of the caller: nothing is kept of it
    const uni_can_protoplexer_msg_t msg = {
        .message_id = message_id,
        .address_from = address_from,
        .address_to = address_to,
        .priority_inverted = priority_inverted,
        .length = length,
        .data = (uint8_t *)(uintptr_t)data,
    };

    for (size_t idx = 0; idx < chunks_count; idx++) {
        const size_t tail = (queue->outbox_head + queue->outbox_count) % storage->outbox_size;
        const uni_can_protoplexer_result_t res =
            uni_can_protoplexer_build_chunk(&msg, queue->protocol.max_chunk_length, idx, &storage->outbox[tail]);
        if (res != UNI_CAN_PROTOPLEXER_OK) {
            // take back what was queued of this message
            queue->outbox_count -= idx;
            return res;
        }
        queue->outbox_count++;
    }

    return UNI_CAN_PROTOPLEXER_OK;
}

uni_can_protoplexer_result_t uni_can_protoplexer_queue_add_chunk(uni_can_protoplexer_queue_t *queue,
                                                                const uni_can_message_t *chunk) {
    if (!queue || !queue->initialized) {
        return UNI_CAN_PROTOPLEXER_ADDING_CHUNK_INVALID_CHUNK;
    }

    uni_can_protoplexer_msg_t msg = {0};
    bool complete = false;
    const uni_can_protoplexer_result_t res = uni_can_protoplexer_add_chunk_view(&queue->protocol, chunk, &msg, &complete);

    if (complete && !uni_can_protoplexer_queue_inbox_push(queue, &msg)) {
        return UNI_CAN_PROTOPLEXER_ADDING_CHUNK_CANT_SAVE_MESSAGE;
    }

    return res;
}

uni_can_protoplexer_result_t uni_can_protoplexer_queue_poll_rx(uni_can_protoplexer_queue_t *queue) {
    if (!queue || !queue->initialized || !queue->config.receive_chunk) {
        return UNI_CAN_PROTOPLEXER_POLLING_RX_HW_RX_EMPTY;
    }

    uni_can_message_t chunk = {0};
    if (!queue->config.receive_chunk(queue->config.cookie, &chunk)) {
        return UNI_CAN_PROTOPLEXER_POLLING_RX_HW_RX_EMPTY;
    }

    return uni_can_protoplexer_queue_add_chunk(queue, &chunk);
}

uni_can_protoplexer_result_t uni_can_protoplexer_queue_poll_tx(uni_can_protoplexer_queue_t *queue) {
    if (!queue || !queue->initialized || queue->outbox_count == 0) {
        return UNI_CAN_PROTOPLEXER_POLLING_TX_BUFFER_EMPTY;
    }

    // a network that cannot send drops its chunks
    uni_can_protoplexer_hw_result_t hw_result = UNI_CAN_PROTOPLEXER_HW_ERROR_NON_RECOVERABLE;
    if (queue->config.send_chunk) {
        hw_result = queue->config.send_chunk(queue->config.cookie, &queue->config.storage->outbox[queue->outbox_head]);
    }

    uni_can_protoplexer_result_t result = UNI_CAN_PROTOPLEXER_FAILURE;
    switch (hw_result) {
        case UNI_CAN_PROTOPLEXER_HW_ERROR_RESEND_REQUIRED:
            queue->send_attempts++;
            if (queue->send_attempts > queue->config.send_attempts) {
                queue->send_attempts = 0;
                result = UNI_CAN_PROTOPLEXER_HW_SEND_TOO_MANY_ATTEMPTS;
            } else {
                result = UNI_CAN_PROTOPLEXER_OK;
            }
            break;

        case UNI_CAN_PROTOPLEXER_HW_OK:
            uni_can_protoplexer_queue_outbox_pop(queue);
            queue->send_attempts = 0;
            result = UNI_CAN_PROTOPLEXER_OK;
            break;

        case UNI_CAN_PROTOPLEXER_HW_ERROR_NON_RECOVERABLE:
            uni_can_protoplexer_queue_outbox_pop(queue);
            queue->send_attempts = 0;
            result = UNI_CAN_PROTOPLEXER_HW_SEND_NON_RECOVERABLE;
            break;

        default:
            break;
    }

    return result;
}

uni_can_protoplexer_result_t uni_can_protoplexer_queue_poll_new(uni_can_protoplexer_queue_t *queue) {
    if (!queue || !queue->initialized || queue->inbox_count == 0) {
        return UNI_CAN_PROTOPLEXER_POLLING_NEW_NO_NEW_MESSAGES;
    }

    const uni_can_protoplexer_queue_storage_t *storage = queue->config.storage;
    if (queue->config.rx_event) {
        queue->config.rx_event(queue->config.cookie, &storage->inbox[queue->inbox_head]);
    }

    queue->inbox_head = (queue->inbox_head + 1U) % storage->inbox_size;
    queue->inbox_count--;

    return UNI_CAN_PROTOPLEXER_OK;
}
