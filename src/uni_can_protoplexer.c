//
// ProtoPlexer protocol (C implementation)
//

// stdlib
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// uni.can
#include "uni_can_protoplexer.h"


//
// CRC16 (poly table 0x1021), initial 0x0000, no final XOR
// (matches reference implementation)
//

static const uint16_t uni_can_protoplexer_crc16tab[] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
};

uint16_t uni_can_protoplexer_crc16(const uint8_t *buf, uint32_t len) {
    uint16_t retval = 0;
    for (uint32_t i = 0; i < len; i++) {
        retval = (uint16_t)(((retval << 8U) & 0xFFFFU) ^ uni_can_protoplexer_crc16tab[((retval >> 8U) ^ buf[i]) & 0x00FFU]);
    }
    return retval;
}


//
// Internal RX buffer
//

typedef struct {
    bool in_use;
    uint32_t channel_signature;

    uint16_t expected_length;
    uint16_t expected_crc;
    uint16_t message_id;

    uint16_t address_from;
    uint16_t address_to;
    uint8_t priority_inverted;

    uint8_t *payload;
    uint16_t payload_size;
} uni_can_protoplexer_rx_buffer_t;

// The public ctx struct is declared in the header. Here we treat ctx->rx as
// uni_can_protoplexer_rx_buffer_t*.


static bool uni_can_protoplexer_address_valid(uint16_t address) {
    return (address >= UNI_CAN_PROTOPLEXER_ADDRESS_MIN) && (address <= UNI_CAN_PROTOPLEXER_ADDRESS_MAX);
}

static uni_can_protoplexer_rx_buffer_t *uni_can_protoplexer_rx_find(uni_can_protoplexer_ctx_t *ctx, uint32_t sig) {
    uni_can_protoplexer_rx_buffer_t *rx = (uni_can_protoplexer_rx_buffer_t *)((ctx) ? ctx->rx : NULL);
    if (!ctx || !rx) {
        return NULL;
    }
    for (uint16_t i = 0; i < ctx->max_channels; i++) {
        if (rx[i].in_use && rx[i].channel_signature == sig) {
            return &rx[i];
        }
    }
    return NULL;
}

static uni_can_protoplexer_rx_buffer_t *uni_can_protoplexer_rx_get_or_alloc(uni_can_protoplexer_ctx_t *ctx, uint32_t sig) {
    uni_can_protoplexer_rx_buffer_t *b = uni_can_protoplexer_rx_find(ctx, sig);
    if (b) {
        return b;
    }

    uni_can_protoplexer_rx_buffer_t *rx = (uni_can_protoplexer_rx_buffer_t *)ctx->rx;
    for (uint16_t i = 0; i < ctx->max_channels; i++) {
        if (!rx[i].in_use) {
            rx[i] = (uni_can_protoplexer_rx_buffer_t){0};
            rx[i].in_use = true;
            rx[i].channel_signature = sig;
            rx[i].message_id = UNI_CAN_PROTOPLEXER_ERROR_ID;
            return &rx[i];
        }
    }
    return NULL;
}

static void uni_can_protoplexer_rx_reset(uni_can_protoplexer_rx_buffer_t *b) {
    if (!b) {
        return;
    }
    free(b->payload);
    b->payload = NULL;
    b->payload_size = 0;
    b->expected_length = 0;
    b->expected_crc = 0;
    b->message_id = UNI_CAN_PROTOPLEXER_ERROR_ID;
    b->address_from = 0;
    b->address_to = 0;
    b->priority_inverted = UNI_CAN_PROTOPLEXER_PRIORITY_MINIMAL;
}

static void uni_can_protoplexer_rx_release(uni_can_protoplexer_rx_buffer_t *b) {
    if (!b) {
        return;
    }
    uni_can_protoplexer_rx_reset(b);
    b->in_use = false;
    b->channel_signature = 0;
}


//
// Public API
//

bool uni_can_protoplexer_init(uni_can_protoplexer_ctx_t *ctx, const uni_can_protoplexer_config_t *cfg) {
    if (!ctx || !cfg) {
        return false;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->own_address = cfg->own_address & UNI_CAN_PROTOPLEXER_ADDRESS_MASK;
    ctx->max_chunk_length = cfg->max_chunk_length;
    ctx->monitoring = cfg->monitoring;
    ctx->max_channels = cfg->max_channels;

    if (ctx->max_channels == 0) {
        ctx->max_channels = 16;
    }

    if (ctx->max_chunk_length < UNI_CAN_PROTOPLEXER_HEADER_SIZE || ctx->max_chunk_length > UNI_CAN_MESSAGE_MAXLEN) {
        return false;
    }

    ctx->rx = calloc(ctx->max_channels, sizeof(uni_can_protoplexer_rx_buffer_t));
    if (!ctx->rx) {
        memset(ctx, 0, sizeof(*ctx));
        return false;
    }

    return true;
}

void uni_can_protoplexer_deinit(uni_can_protoplexer_ctx_t *ctx) {
    if (!ctx) {
        return;
    }
    uni_can_protoplexer_rx_buffer_t *rx = (uni_can_protoplexer_rx_buffer_t *)ctx->rx;
    if (rx) {
        for (uint16_t i = 0; i < ctx->max_channels; i++) {
            if (rx[i].in_use) {
                uni_can_protoplexer_rx_release(&rx[i]);
            }
        }
        free(rx);
    }
    memset(ctx, 0, sizeof(*ctx));
}

const char *uni_can_protoplexer_result_to_string(uni_can_protoplexer_result_t value) {
    switch (value) {
        case UNI_CAN_PROTOPLEXER_OK: return "ok";
        case UNI_CAN_PROTOPLEXER_FAILURE: return "failure";
        case UNI_CAN_PROTOPLEXER_WRONG_ADDRESS: return "wrong_address";
        case UNI_CAN_PROTOPLEXER_WRONG_PRIORITY: return "wrong_priority";
        case UNI_CAN_PROTOPLEXER_ADDING_CHUNK_CANT_ALLOCATE_NEW_MSG: return "adding_chunk_cant_allocate_new_msg";
        case UNI_CAN_PROTOPLEXER_ADDING_CHUNK_FIRST_AND_TOO_SHORT: return "adding_chunk_first_and_too_short";
        case UNI_CAN_PROTOPLEXER_ADDING_CHUNK_UNRELATED_ADDRESS_TO: return "adding_chunk_unrelated_adress_to";
        case UNI_CAN_PROTOPLEXER_ADDING_CHUNK_HEADER_DATA_OVERFLOW: return "adding_chunk_header_data_overflow";
        case UNI_CAN_PROTOPLEXER_ADDING_CHUNK_CANT_ALLOCATE_DATA: return "adding_chunk_cant_allocate_data";
        case UNI_CAN_PROTOPLEXER_ADDING_CHUNK_RECEIVED_ERROR_ID: return "adding_chunk_received_error_ID";
        case UNI_CAN_PROTOPLEXER_ADDING_CHUNK_DATA_WITHOUT_HEADER: return "adding_chunk_data_without_header";
        case UNI_CAN_PROTOPLEXER_ADDING_CHUNK_DATA_OVERFLOW: return "adding_chunk_data_overflow";
        case UNI_CAN_PROTOPLEXER_ADDING_CHUNK_CANT_SAVE_DATA: return "adding_chunk_cant_save_data";
        case UNI_CAN_PROTOPLEXER_ADDING_CHUNK_CANT_SAVE_MESSAGE: return "adding_chunk_cant_save_message";
        case UNI_CAN_PROTOPLEXER_ADDING_CHUNK_INVALID_CHUNK: return "adding_chunk_invalid_chunk";
        case UNI_CAN_PROTOPLEXER_ADDING_CHUNK_WRONG_CRC: return "adding_chunk_wrong_crc";
        case UNI_CAN_PROTOPLEXER_SEND_MSG_HW_CANT_SEND_HEADER: return "send_msg_hw_cant_send_header";
        case UNI_CAN_PROTOPLEXER_SEND_MSG_CANT_ADD_MESSAGE_CHUNKS: return "send_msg_cant_add_message_chunks";
        case UNI_CAN_PROTOPLEXER_SEND_MSG_CANT_WRITE_HEADER: return "send_msg_cant_write_header";
        default: return "unknown";
    }
}

uint32_t uni_can_protoplexer_canid_create(uint16_t address_from, uint16_t address_to, uint8_t priority_inverted, bool is_first) {
    uint32_t can_id = ((uint32_t)priority_inverted & 0x0FU) << 25U;
    if (is_first) {
        can_id |= 1U << 24U;
    }
    can_id |= ((uint32_t)address_from & 0x0FFFU) << 12U;
    can_id |= ((uint32_t)address_to & 0x0FFFU);
    return can_id;
}

uint16_t uni_can_protoplexer_canid_get_from(uint32_t can_id) {
    return (uint16_t)((can_id >> 12U) & UNI_CAN_PROTOPLEXER_ADDRESS_MASK);
}

uint16_t uni_can_protoplexer_canid_get_to(uint32_t can_id) {
    return (uint16_t)(can_id & UNI_CAN_PROTOPLEXER_ADDRESS_MASK);
}

uint8_t uni_can_protoplexer_canid_get_priority_inverted(uint32_t can_id) {
    return (uint8_t)((can_id >> 25U) & UNI_CAN_PROTOPLEXER_PRIORITY_MASK);
}

bool uni_can_protoplexer_canid_get_is_first(uint32_t can_id) {
    return ((can_id >> 24U) & 0x01U) == 0x01U;
}


uni_can_protoplexer_msg_t *uni_can_protoplexer_msg_create(uint16_t message_id, uint16_t address_from, uint16_t address_to,
                                                          uint8_t priority_inverted, const uint8_t *data, uint16_t length) {
    uni_can_protoplexer_msg_t *m = (uni_can_protoplexer_msg_t *)calloc(1, sizeof(uni_can_protoplexer_msg_t));
    if (!m) {
        return NULL;
    }
    m->message_id = message_id;
    m->address_from = address_from;
    m->address_to = address_to;
    m->priority_inverted = priority_inverted;
    m->length = length;
    if (length > 0) {
        m->data = (uint8_t *)malloc(length);
        if (!m->data) {
            free(m);
            return NULL;
        }
        memcpy(m->data, data, length);
    }
    return m;
}

void uni_can_protoplexer_msg_free(uni_can_protoplexer_msg_t *msg) {
    if (!msg) {
        return;
    }
    free(msg->data);
    free(msg);
}


uni_can_protoplexer_result_t uni_can_protoplexer_build_chunks(const uni_can_protoplexer_msg_t *msg,
                                                              uint16_t max_chunk_length,
                                                              uni_can_message_t **out_chunks,
                                                              size_t *out_count) {
    if (!msg || !out_chunks || !out_count) {
        return UNI_CAN_PROTOPLEXER_FAILURE;
    }
    *out_chunks = NULL;
    *out_count = 0;

    if (max_chunk_length < UNI_CAN_PROTOPLEXER_HEADER_SIZE || max_chunk_length > UNI_CAN_MESSAGE_MAXLEN) {
        return UNI_CAN_PROTOPLEXER_SEND_MSG_HW_CANT_SEND_HEADER;
    }

    // Total bytes = header + payload
    const uint32_t total = (uint32_t)UNI_CAN_PROTOPLEXER_HEADER_SIZE + (uint32_t)msg->length;

    size_t chunks_count = (total + (max_chunk_length - 1U)) / max_chunk_length;
    if (chunks_count == 0) {
        chunks_count = 1;
    }

    uni_can_message_t *chunks = (uni_can_message_t *)calloc(chunks_count, sizeof(uni_can_message_t));
    if (!chunks) {
        return UNI_CAN_PROTOPLEXER_SEND_MSG_CANT_ADD_MESSAGE_CHUNKS;
    }

    // Prepare header bytes
    uint8_t header[UNI_CAN_PROTOPLEXER_HEADER_SIZE];
    const uint16_t crc = uni_can_protoplexer_crc16(msg->data, msg->length);
    header[UNI_CAN_PROTOPLEXER_HEADER_ID_OFFSET] = (uint8_t)(msg->message_id & 0x00FFU);
    header[UNI_CAN_PROTOPLEXER_HEADER_ID_OFFSET + 1] = (uint8_t)(msg->message_id >> 8U);
    header[UNI_CAN_PROTOPLEXER_HEADER_LENGTH_OFFSET] = (uint8_t)(msg->length & 0x00FFU);
    header[UNI_CAN_PROTOPLEXER_HEADER_LENGTH_OFFSET + 1] = (uint8_t)(msg->length >> 8U);
    header[UNI_CAN_PROTOPLEXER_HEADER_CRC_OFFSET] = (uint8_t)(crc & 0x00FFU);
    header[UNI_CAN_PROTOPLEXER_HEADER_CRC_OFFSET + 1] = (uint8_t)(crc >> 8U);

    uint32_t left = total;
    uint32_t cursor = 0; // cursor in header+payload stream

    for (size_t idx = 0; idx < chunks_count; idx++) {
        uint16_t chunk_size = max_chunk_length;
        if (left < max_chunk_length) {
            chunk_size = (uint16_t)left;
        }

        uni_can_message_t *ch = &chunks[idx];
        ch->flags = UNI_CAN_MSG_FLAG_EXT_ID;
        ch->len = chunk_size;

        const bool is_first = (idx == 0);
        ch->id = uni_can_protoplexer_canid_create(msg->address_from, msg->address_to, msg->priority_inverted, is_first);

        // Fill chunk bytes
        for (uint16_t i = 0; i < chunk_size; i++) {
            uint32_t pos = cursor + i;
            if (pos < UNI_CAN_PROTOPLEXER_HEADER_SIZE) {
                ch->data.u8[i] = header[pos];
            } else {
                uint32_t p = pos - UNI_CAN_PROTOPLEXER_HEADER_SIZE;
                ch->data.u8[i] = msg->data[p];
            }
        }

        cursor += chunk_size;
        left -= chunk_size;
    }

    *out_chunks = chunks;
    *out_count = chunks_count;
    return UNI_CAN_PROTOPLEXER_OK;
}

void uni_can_protoplexer_free_chunks(uni_can_message_t *chunks) {
    free(chunks);
}


uni_can_protoplexer_result_t uni_can_protoplexer_add_chunk(uni_can_protoplexer_ctx_t *ctx,
                                                          const uni_can_message_t *chunk,
                                                          uni_can_protoplexer_msg_t **out_msg) {
    if (out_msg) {
        *out_msg = NULL;
    }

    if (!ctx || !chunk) {
        return UNI_CAN_PROTOPLEXER_ADDING_CHUNK_INVALID_CHUNK;
    }

    const uint16_t addr_to = uni_can_protoplexer_canid_get_to(chunk->id);
    const uint16_t addr_from = uni_can_protoplexer_canid_get_from(chunk->id);
    const uint8_t prio = uni_can_protoplexer_canid_get_priority_inverted(chunk->id);
    const bool is_first = uni_can_protoplexer_canid_get_is_first(chunk->id);

    const uint32_t sig = (uint32_t)addr_to | ((uint32_t)addr_from << 12U);

    // ignore unrelated chunks
    if ((addr_to != ctx->own_address) && (!ctx->monitoring)) {
        return UNI_CAN_PROTOPLEXER_ADDING_CHUNK_UNRELATED_ADDRESS_TO;
    }

    uni_can_protoplexer_rx_buffer_t *b = uni_can_protoplexer_rx_get_or_alloc(ctx, sig);
    if (!b) {
        return UNI_CAN_PROTOPLEXER_ADDING_CHUNK_CANT_ALLOCATE_NEW_MSG;
    }

    // reset message if needed (header overwrite)
    if (is_first && (b->message_id != UNI_CAN_PROTOPLEXER_ERROR_ID)) {
        uni_can_protoplexer_rx_reset(b);
    }

    // prepare msg in case of first chunk
    if (is_first) {
        if (chunk->len < UNI_CAN_PROTOPLEXER_CHUNK_STARTING_LENGTH_MIN) {
            uni_can_protoplexer_rx_release(b);
            return UNI_CAN_PROTOPLEXER_ADDING_CHUNK_FIRST_AND_TOO_SHORT;
        }

        if (!uni_can_protoplexer_address_valid(addr_to) || !uni_can_protoplexer_address_valid(addr_from)) {
            // match reference "setter" validation behaviour
            uni_can_protoplexer_rx_release(b);
            return UNI_CAN_PROTOPLEXER_WRONG_ADDRESS;
        }

        if (prio > UNI_CAN_PROTOPLEXER_PRIORITY_MINIMAL) {
            uni_can_protoplexer_rx_release(b);
            return UNI_CAN_PROTOPLEXER_WRONG_PRIORITY;
        }

        b->address_to = addr_to;
        b->address_from = addr_from;
        b->priority_inverted = prio;

        b->expected_length = (uint16_t)chunk->data.u8[UNI_CAN_PROTOPLEXER_HEADER_LENGTH_OFFSET] |
                             (uint16_t)(chunk->data.u8[UNI_CAN_PROTOPLEXER_HEADER_LENGTH_OFFSET + 1] << 8U);
        b->expected_crc = (uint16_t)chunk->data.u8[UNI_CAN_PROTOPLEXER_HEADER_CRC_OFFSET] |
                          (uint16_t)(chunk->data.u8[UNI_CAN_PROTOPLEXER_HEADER_CRC_OFFSET + 1] << 8U);

        if ((uint16_t)(chunk->len - UNI_CAN_PROTOPLEXER_HEADER_DATA_OFFSET) > b->expected_length) {
            uni_can_protoplexer_rx_release(b);
            return UNI_CAN_PROTOPLEXER_ADDING_CHUNK_HEADER_DATA_OVERFLOW;
        }

        // allocate payload buffer (exact expected size)
        free(b->payload);
        b->payload = NULL;
        b->payload_size = 0;

        if (b->expected_length > 0) {
            b->payload = (uint8_t *)malloc(b->expected_length);
            if (!b->payload) {
                uni_can_protoplexer_rx_release(b);
                return UNI_CAN_PROTOPLEXER_ADDING_CHUNK_CANT_ALLOCATE_DATA;
            }
        }

        // copy first chunk payload bytes (after header)
        const uint16_t embedded = (uint16_t)(chunk->len - UNI_CAN_PROTOPLEXER_HEADER_DATA_OFFSET);
        if (embedded > 0) {
            memcpy(b->payload, &chunk->data.u8[UNI_CAN_PROTOPLEXER_HEADER_DATA_OFFSET], embedded);
            b->payload_size = embedded;
        }

        b->message_id = (uint16_t)chunk->data.u8[UNI_CAN_PROTOPLEXER_HEADER_ID_OFFSET] |
                        (uint16_t)(chunk->data.u8[UNI_CAN_PROTOPLEXER_HEADER_ID_OFFSET + 1] << 8U);

        if (b->message_id == UNI_CAN_PROTOPLEXER_ERROR_ID) {
            uni_can_protoplexer_rx_release(b);
            return UNI_CAN_PROTOPLEXER_ADDING_CHUNK_RECEIVED_ERROR_ID;
        }
    }

    // Handle data chunks
    if (!is_first) {
        if (b->message_id == UNI_CAN_PROTOPLEXER_ERROR_ID) {
            uni_can_protoplexer_rx_release(b);
            return UNI_CAN_PROTOPLEXER_ADDING_CHUNK_DATA_WITHOUT_HEADER;
        }

        if ((uint32_t)b->payload_size + (uint32_t)chunk->len > (uint32_t)b->expected_length) {
            uni_can_protoplexer_rx_release(b);
            return UNI_CAN_PROTOPLEXER_ADDING_CHUNK_DATA_OVERFLOW;
        }

        if (chunk->len > 0) {
            memcpy(&b->payload[b->payload_size], chunk->data.u8, chunk->len);
            b->payload_size = (uint16_t)(b->payload_size + chunk->len);
        }
    }

    // finishing
    if (b->payload_size == b->expected_length) {
        if (uni_can_protoplexer_crc16(b->payload, b->expected_length) != b->expected_crc) {
            // IMPORTANT: match reference edge-case behaviour
            // do NOT clear/release buffer on CRC mismatch.
            return UNI_CAN_PROTOPLEXER_ADDING_CHUNK_WRONG_CRC;
        }

        if (out_msg) {
            uni_can_protoplexer_msg_t *m = uni_can_protoplexer_msg_create(b->message_id, b->address_from, b->address_to,
                                                                          b->priority_inverted, b->payload, b->expected_length);
            if (!m) {
                uni_can_protoplexer_rx_release(b);
                return UNI_CAN_PROTOPLEXER_ADDING_CHUNK_CANT_SAVE_MESSAGE;
            }
            *out_msg = m;
        }

        uni_can_protoplexer_rx_release(b);
    }

    return UNI_CAN_PROTOPLEXER_OK;
}
