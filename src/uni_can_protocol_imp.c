#include "uni_can_protocol_imp.h"

#include <stdlib.h>
#include <string.h>

typedef enum {
    UNI_CAN_IMP_SESSION_FREE = 0,
    UNI_CAN_IMP_SESSION_RX,
    UNI_CAN_IMP_SESSION_TX_WITHOUT_ACK,
    UNI_CAN_IMP_SESSION_TX_WAIT_START_ACK,
    UNI_CAN_IMP_SESSION_TX_WAIT_DATA_ACK,
    UNI_CAN_IMP_SESSION_TX_WAIT_END_ACK,
} uni_can_imp_session_state_t;

typedef struct {
    uint32_t address_mask;
    uni_can_imp_session_state_t state;
    uni_can_imp_msg_mode_t mode;
    uint8_t peer_address;
    uint8_t local_address;
    uint8_t session_id;
    uint8_t last_data_length;
    uint8_t *payload;
    uint16_t payload_size;
    uint16_t payload_capacity;
    uint16_t payload_offset;
    uint32_t last_activity_ms;
} uni_can_imp_session_t;

static bool uni_can_imp_address_valid(uint8_t address) {
    return address <= UNI_CAN_IMP_DEVICE_ID_MAX;
}

static bool uni_can_imp_source_address_valid(uint8_t address) {
    return uni_can_imp_address_valid(address) && address != UNI_CAN_IMP_DEVICE_ID_ALL;
}

static uni_can_imp_session_t *uni_can_imp_sessions(uni_can_imp_ctx_t *ctx) {
    return ctx ? (uni_can_imp_session_t *)ctx->sessions : NULL;
}

static void uni_can_imp_session_release(uni_can_imp_session_t *session) {
    if (!session) {
        return;
    }

    const uint32_t address_mask = session->address_mask;
    free(session->payload);
    memset(session, 0, sizeof(*session));
    session->address_mask = address_mask;
}

static bool uni_can_imp_session_accepts(const uni_can_imp_session_t *session, uint8_t peer_address) {
    return session && (session->address_mask & ((uint32_t)1U << peer_address)) != 0U;
}

static uni_can_imp_session_t *uni_can_imp_find_session(uni_can_imp_ctx_t *ctx, uint8_t peer_address,
                                                        uint8_t local_address, uint8_t session_id) {
    uni_can_imp_session_t *sessions = uni_can_imp_sessions(ctx);
    if (!sessions) {
        return NULL;
    }

    for (uint16_t i = 0; i < ctx->max_sessions; i++) {
        uni_can_imp_session_t *session = &sessions[i];
        if (session->state != UNI_CAN_IMP_SESSION_FREE && session->peer_address == peer_address &&
            session->local_address == local_address && session->session_id == session_id) {
            return session;
        }
    }
    return NULL;
}

static uni_can_imp_session_t *uni_can_imp_find_free_session(uni_can_imp_ctx_t *ctx, uint8_t peer_address) {
    uni_can_imp_session_t *sessions = uni_can_imp_sessions(ctx);
    if (!sessions) {
        return NULL;
    }

    for (uint16_t i = 0; i < ctx->max_sessions; i++) {
        if (sessions[i].state == UNI_CAN_IMP_SESSION_FREE && uni_can_imp_session_accepts(&sessions[i], peer_address)) {
            return &sessions[i];
        }
    }
    return NULL;
}

static bool uni_can_imp_session_is_tx(const uni_can_imp_session_t *session) {
    return session && session->state >= UNI_CAN_IMP_SESSION_TX_WITHOUT_ACK;
}

static bool uni_can_imp_command_without_ack(uni_can_imp_cmd_t command) {
    return command == UNI_CAN_IMP_CMD_START_WA || command == UNI_CAN_IMP_CMD_DATA_WA ||
           command == UNI_CAN_IMP_CMD_END_WA || command == UNI_CAN_IMP_CMD_ABORT_WA;
}

static void uni_can_imp_set_event(uni_can_imp_output_t *output, uni_can_imp_event_t event,
                                  const uni_can_imp_session_t *session) {
    output->event = event;
    output->peer_address = session->peer_address;
    output->session_id = session->session_id;
}

static void uni_can_imp_set_frame(uni_can_imp_ctx_t *ctx, uni_can_imp_output_t *output, uint8_t address_to,
                                  uint8_t session_id, uni_can_imp_cmd_t command, const uint8_t *data, uint8_t length) {
    output->frame_ready = true;
    memset(&output->frame, 0, sizeof(output->frame));
    output->frame.id = uni_can_imp_canid_create(ctx->own_address, address_to, session_id, command);
    output->frame.flags = UNI_CAN_MSG_FLAG_EXT_ID;
    output->frame.len = length;
    if (length > 0U) {
        memcpy(output->frame.data.u8, data, length);
    }
}

static uni_can_imp_result_t uni_can_imp_session_copy_payload(uni_can_imp_session_t *session, const uint8_t *data,
                                                              uint16_t length) {
    if (length == 0U) {
        return UNI_CAN_IMP_OK;
    }

    session->payload = (uint8_t *)malloc(length);
    if (!session->payload) {
        return UNI_CAN_IMP_CANT_ALLOCATE;
    }
    memcpy(session->payload, data, length);
    session->payload_size = length;
    session->payload_capacity = length;
    return UNI_CAN_IMP_OK;
}

static uni_can_imp_result_t uni_can_imp_session_append(uni_can_imp_ctx_t *ctx, uni_can_imp_session_t *session,
                                                        const uint8_t *data, uint16_t length) {
    const uint32_t required = (uint32_t)session->payload_size + length;
    if (required > ctx->max_message_size) {
        return UNI_CAN_IMP_MESSAGE_TOO_LARGE;
    }
    if (length == 0U) {
        return UNI_CAN_IMP_OK;
    }

    if (required > session->payload_capacity) {
        uint32_t capacity = session->payload_capacity ? (uint32_t)session->payload_capacity * 2U : 32U;
        if (capacity < required) {
            capacity = required;
        }
        if (capacity > ctx->max_message_size) {
            capacity = ctx->max_message_size;
        }

        uint8_t *payload = (uint8_t *)realloc(session->payload, capacity);
        if (!payload) {
            return UNI_CAN_IMP_CANT_ALLOCATE;
        }
        session->payload = payload;
        session->payload_capacity = (uint16_t)capacity;
    }

    memcpy(session->payload + session->payload_size, data, length);
    session->payload_size = (uint16_t)required;
    return UNI_CAN_IMP_OK;
}

static void uni_can_imp_emit_abort(uni_can_imp_ctx_t *ctx, uni_can_imp_output_t *output, uint8_t peer_address,
                                   uint8_t session_id) {
    uni_can_imp_set_frame(ctx, output, peer_address, session_id, UNI_CAN_IMP_CMD_ABORT_WA, NULL, 0);
}

static void uni_can_imp_emit_next_acked_frame(uni_can_imp_ctx_t *ctx, uni_can_imp_session_t *session,
                                               uint32_t now_ms, uni_can_imp_output_t *output) {
    if (session->payload_offset < session->payload_size) {
        const uint16_t bytes_left = (uint16_t)(session->payload_size - session->payload_offset);
        const uint8_t length = (uint8_t)(bytes_left > UNI_CAN_MESSAGE_MAXLEN ? UNI_CAN_MESSAGE_MAXLEN : bytes_left);
        session->last_data_length = length;
        session->state = UNI_CAN_IMP_SESSION_TX_WAIT_DATA_ACK;
        session->last_activity_ms = now_ms;
        uni_can_imp_set_frame(ctx, output, session->peer_address, session->session_id, UNI_CAN_IMP_CMD_DATA,
                              session->payload + session->payload_offset, length);
        return;
    }

    session->state = UNI_CAN_IMP_SESSION_TX_WAIT_END_ACK;
    session->last_activity_ms = now_ms;
    uni_can_imp_set_frame(ctx, output, session->peer_address, session->session_id, UNI_CAN_IMP_CMD_END, NULL, 0);
}

static void uni_can_imp_resend_acked_frame(uni_can_imp_ctx_t *ctx, uni_can_imp_session_t *session,
                                           uint32_t now_ms, uni_can_imp_output_t *output) {
    session->last_activity_ms = now_ms;
    if (session->state == UNI_CAN_IMP_SESSION_TX_WAIT_START_ACK) {
        uni_can_imp_set_frame(ctx, output, session->peer_address, session->session_id, UNI_CAN_IMP_CMD_START, NULL, 0);
    } else if (session->state == UNI_CAN_IMP_SESSION_TX_WAIT_DATA_ACK) {
        uni_can_imp_set_frame(ctx, output, session->peer_address, session->session_id, UNI_CAN_IMP_CMD_DATA,
                              session->payload + session->payload_offset, session->last_data_length);
    }
}

static uni_can_imp_result_t uni_can_imp_handle_tx_frame(uni_can_imp_ctx_t *ctx, uni_can_imp_session_t *session,
                                                         uni_can_imp_cmd_t command, uint32_t now_ms,
                                                         uni_can_imp_output_t *output) {
    if (command == UNI_CAN_IMP_CMD_ABORT_WA) {
        uni_can_imp_set_event(output, UNI_CAN_IMP_EVENT_TX_ABORTED, session);
        uni_can_imp_session_release(session);
        return UNI_CAN_IMP_OK;
    }

    if (command == UNI_CAN_IMP_CMD_NACK) {
        if (session->state == UNI_CAN_IMP_SESSION_TX_WAIT_START_ACK ||
            session->state == UNI_CAN_IMP_SESSION_TX_WAIT_DATA_ACK) {
            uni_can_imp_resend_acked_frame(ctx, session, now_ms, output);
            return UNI_CAN_IMP_OK;
        }

        uni_can_imp_set_event(output, UNI_CAN_IMP_EVENT_TX_ABORTED, session);
        uni_can_imp_session_release(session);
        return UNI_CAN_IMP_MESSAGE_REJECTED;
    }

    if (command != UNI_CAN_IMP_CMD_ACK) {
        uni_can_imp_emit_abort(ctx, output, session->peer_address, session->session_id);
        uni_can_imp_set_event(output, UNI_CAN_IMP_EVENT_TX_ABORTED, session);
        uni_can_imp_session_release(session);
        return UNI_CAN_IMP_UNEXPECTED_COMMAND;
    }

    if (session->state == UNI_CAN_IMP_SESSION_TX_WAIT_START_ACK) {
        uni_can_imp_emit_next_acked_frame(ctx, session, now_ms, output);
        return UNI_CAN_IMP_OK;
    }
    if (session->state == UNI_CAN_IMP_SESSION_TX_WAIT_DATA_ACK) {
        session->payload_offset = (uint16_t)(session->payload_offset + session->last_data_length);
        uni_can_imp_emit_next_acked_frame(ctx, session, now_ms, output);
        return UNI_CAN_IMP_OK;
    }
    if (session->state == UNI_CAN_IMP_SESSION_TX_WAIT_END_ACK) {
        uni_can_imp_set_event(output, UNI_CAN_IMP_EVENT_TX_FINISHED, session);
        uni_can_imp_session_release(session);
        return UNI_CAN_IMP_OK;
    }

    return UNI_CAN_IMP_UNEXPECTED_COMMAND;
}

static uni_can_imp_result_t uni_can_imp_finish_rx(uni_can_imp_ctx_t *ctx, uni_can_imp_session_t *session,
                                                  uni_can_imp_cmd_t command, uni_can_imp_output_t *output) {
    uni_can_imp_msg_t candidate;
    memset(&candidate, 0, sizeof(candidate));
    candidate.mode = session->mode;
    candidate.address_from = session->peer_address;
    candidate.address_to = session->local_address;
    candidate.session_id = session->session_id;
    candidate.length = session->payload_size;
    candidate.data = session->payload;

    if (ctx->message_validator && !ctx->message_validator(&candidate, ctx->user_data)) {
        if (command == UNI_CAN_IMP_CMD_END) {
            uni_can_imp_set_frame(ctx, output, session->peer_address, session->session_id, UNI_CAN_IMP_CMD_NACK, NULL, 0);
        }
        uni_can_imp_set_event(output, UNI_CAN_IMP_EVENT_RX_ABORTED, session);
        uni_can_imp_session_release(session);
        return UNI_CAN_IMP_MESSAGE_REJECTED;
    }

    uni_can_imp_msg_t *message = uni_can_imp_msg_create(candidate.mode, candidate.address_from, candidate.address_to,
                                                        candidate.data, candidate.length);
    if (!message) {
        if (command == UNI_CAN_IMP_CMD_END) {
            uni_can_imp_emit_abort(ctx, output, session->peer_address, session->session_id);
        }
        uni_can_imp_set_event(output, UNI_CAN_IMP_EVENT_RX_ABORTED, session);
        uni_can_imp_session_release(session);
        return UNI_CAN_IMP_CANT_ALLOCATE;
    }
    message->session_id = candidate.session_id;

    if (command == UNI_CAN_IMP_CMD_END) {
        uni_can_imp_set_frame(ctx, output, session->peer_address, session->session_id, UNI_CAN_IMP_CMD_ACK, NULL, 0);
    }
    output->received_message = message;
    uni_can_imp_set_event(output, UNI_CAN_IMP_EVENT_RX_FINISHED, session);
    uni_can_imp_session_release(session);
    return UNI_CAN_IMP_OK;
}

static uni_can_imp_result_t uni_can_imp_handle_rx_frame(uni_can_imp_ctx_t *ctx, uni_can_imp_session_t *session,
                                                         const uni_can_message_t *frame, uni_can_imp_cmd_t command,
                                                         uint32_t now_ms, uni_can_imp_output_t *output) {
    if (command == UNI_CAN_IMP_CMD_ABORT_WA) {
        uni_can_imp_set_event(output, UNI_CAN_IMP_EVENT_RX_ABORTED, session);
        uni_can_imp_session_release(session);
        return UNI_CAN_IMP_OK;
    }

    if (command == UNI_CAN_IMP_CMD_START || command == UNI_CAN_IMP_CMD_START_WA) {
        free(session->payload);
        session->payload = NULL;
        session->payload_size = 0;
        session->payload_capacity = 0;
        session->payload_offset = 0;
        session->mode = command == UNI_CAN_IMP_CMD_START ? UNI_CAN_IMP_MSG_MODE_WITH_ACK
                                                         : UNI_CAN_IMP_MSG_MODE_WITHOUT_ACK;
        session->last_activity_ms = now_ms;
        if (command == UNI_CAN_IMP_CMD_START && session->local_address != UNI_CAN_IMP_DEVICE_ID_ALL) {
            uni_can_imp_set_frame(ctx, output, session->peer_address, session->session_id, UNI_CAN_IMP_CMD_ACK, NULL, 0);
        }
        return UNI_CAN_IMP_OK;
    }

    if (command == UNI_CAN_IMP_CMD_DATA || command == UNI_CAN_IMP_CMD_DATA_WA) {
        const uni_can_imp_result_t result =
            uni_can_imp_session_append(ctx, session, frame->data.u8, frame->len);
        if (result != UNI_CAN_IMP_OK) {
            if (command == UNI_CAN_IMP_CMD_DATA) {
                uni_can_imp_emit_abort(ctx, output, session->peer_address, session->session_id);
            }
            uni_can_imp_set_event(output, UNI_CAN_IMP_EVENT_RX_ABORTED, session);
            uni_can_imp_session_release(session);
            return result;
        }

        session->last_activity_ms = now_ms;
        if (command == UNI_CAN_IMP_CMD_DATA) {
            uni_can_imp_set_frame(ctx, output, session->peer_address, session->session_id, UNI_CAN_IMP_CMD_ACK, NULL, 0);
        }
        return UNI_CAN_IMP_OK;
    }

    if (command == UNI_CAN_IMP_CMD_END || command == UNI_CAN_IMP_CMD_END_WA) {
        return uni_can_imp_finish_rx(ctx, session, command, output);
    }

    uni_can_imp_emit_abort(ctx, output, session->peer_address, session->session_id);
    uni_can_imp_set_event(output, UNI_CAN_IMP_EVENT_RX_ABORTED, session);
    uni_can_imp_session_release(session);
    return UNI_CAN_IMP_UNEXPECTED_COMMAND;
}

const char *uni_can_imp_result_to_string(uni_can_imp_result_t value) {
    switch (value) {
        case UNI_CAN_IMP_OK: return "ok";
        case UNI_CAN_IMP_FAILURE: return "failure";
        case UNI_CAN_IMP_INVALID_ARGUMENT: return "invalid_argument";
        case UNI_CAN_IMP_WRONG_ADDRESS: return "wrong_address";
        case UNI_CAN_IMP_INVALID_FRAME: return "invalid_frame";
        case UNI_CAN_IMP_NO_FREE_SESSION: return "no_free_session";
        case UNI_CAN_IMP_CANT_ALLOCATE: return "cant_allocate";
        case UNI_CAN_IMP_MESSAGE_TOO_LARGE: return "message_too_large";
        case UNI_CAN_IMP_UNEXPECTED_COMMAND: return "unexpected_command";
        case UNI_CAN_IMP_MESSAGE_REJECTED: return "message_rejected";
        default: return "unknown";
    }
}

uint32_t uni_can_imp_canid_create(uint8_t address_from, uint8_t address_to, uint8_t session_id,
                                  uni_can_imp_cmd_t command) {
    return ((uint32_t)address_to & 0x1FU) << 24U | ((uint32_t)address_from & 0x1FU) << 19U |
           ((uint32_t)session_id & 0x0FU) << 15U | ((uint32_t)command & 0x1FU) << 10U;
}

uint8_t uni_can_imp_canid_get_from(uint32_t can_id) {
    return (uint8_t)((can_id >> 19U) & 0x1FU);
}

uint8_t uni_can_imp_canid_get_to(uint32_t can_id) {
    return (uint8_t)((can_id >> 24U) & 0x1FU);
}

uint8_t uni_can_imp_canid_get_session_id(uint32_t can_id) {
    return (uint8_t)((can_id >> 15U) & 0x0FU);
}

uni_can_imp_cmd_t uni_can_imp_canid_get_command(uint32_t can_id) {
    return (uni_can_imp_cmd_t)((can_id >> 10U) & 0x1FU);
}

uni_can_imp_msg_t *uni_can_imp_msg_create(uni_can_imp_msg_mode_t mode, uint8_t address_from, uint8_t address_to,
                                          const uint8_t *data, uint16_t length) {
    if ((mode != UNI_CAN_IMP_MSG_MODE_WITH_ACK && mode != UNI_CAN_IMP_MSG_MODE_WITHOUT_ACK) ||
        !uni_can_imp_source_address_valid(address_from) || !uni_can_imp_address_valid(address_to) ||
        (length > 0U && !data)) {
        return NULL;
    }

    uni_can_imp_msg_t *message = (uni_can_imp_msg_t *)calloc(1, sizeof(*message));
    if (!message) {
        return NULL;
    }

    message->mode = address_to == UNI_CAN_IMP_DEVICE_ID_ALL ? UNI_CAN_IMP_MSG_MODE_WITHOUT_ACK : mode;
    message->address_from = address_from;
    message->address_to = address_to;
    message->length = length;
    if (length > 0U) {
        message->data = (uint8_t *)malloc(length);
        if (!message->data) {
            free(message);
            return NULL;
        }
        memcpy(message->data, data, length);
    }
    return message;
}

void uni_can_imp_msg_free(uni_can_imp_msg_t *msg) {
    if (!msg) {
        return;
    }
    free(msg->data);
    free(msg);
}

bool uni_can_imp_init(uni_can_imp_ctx_t *ctx, const uni_can_imp_config_t *cfg) {
    if (!ctx || !cfg || !uni_can_imp_source_address_valid(cfg->own_address)) {
        return false;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->own_address = cfg->own_address;
    ctx->max_sessions = cfg->max_sessions ? cfg->max_sessions : UNI_CAN_IMP_DEFAULT_MAX_SESSIONS;
    ctx->max_message_size =
        cfg->max_message_size ? cfg->max_message_size : UNI_CAN_IMP_DEFAULT_MAX_MESSAGE_SIZE;
    ctx->session_timeout_ms =
        cfg->session_timeout_ms ? cfg->session_timeout_ms : UNI_CAN_IMP_DEFAULT_SESSION_TIMEOUT_MS;
    ctx->message_validator = cfg->message_validator;
    ctx->user_data = cfg->user_data;

    uni_can_imp_session_t *sessions = (uni_can_imp_session_t *)calloc(ctx->max_sessions, sizeof(*sessions));
    if (!sessions) {
        memset(ctx, 0, sizeof(*ctx));
        return false;
    }

    for (uint16_t i = 0; i < ctx->max_sessions; i++) {
        sessions[i].address_mask = UINT32_MAX;
        if (cfg->session_address_masks && i < cfg->session_address_masks_count) {
            sessions[i].address_mask = cfg->session_address_masks[i];
        }
    }
    ctx->sessions = sessions;
    return true;
}

void uni_can_imp_deinit(uni_can_imp_ctx_t *ctx) {
    if (!ctx) {
        return;
    }

    uni_can_imp_session_t *sessions = uni_can_imp_sessions(ctx);
    if (sessions) {
        for (uint16_t i = 0; i < ctx->max_sessions; i++) {
            free(sessions[i].payload);
        }
        free(sessions);
    }
    memset(ctx, 0, sizeof(*ctx));
}

void uni_can_imp_output_clear(uni_can_imp_output_t *output) {
    if (output) {
        memset(output, 0, sizeof(*output));
    }
}

uni_can_imp_result_t uni_can_imp_start_send(uni_can_imp_ctx_t *ctx, const uni_can_imp_msg_t *msg,
                                            uint32_t now_ms, uni_can_imp_output_t *output) {
    if (!ctx || !ctx->sessions || !msg || !output) {
        return UNI_CAN_IMP_INVALID_ARGUMENT;
    }
    uni_can_imp_output_clear(output);

    if (msg->mode != UNI_CAN_IMP_MSG_MODE_WITH_ACK && msg->mode != UNI_CAN_IMP_MSG_MODE_WITHOUT_ACK) {
        return UNI_CAN_IMP_INVALID_ARGUMENT;
    }
    if (msg->address_from != ctx->own_address || !uni_can_imp_address_valid(msg->address_to) ||
        msg->address_to == ctx->own_address || msg->length > ctx->max_message_size ||
        (msg->length > 0U && !msg->data)) {
        return msg->length > ctx->max_message_size ? UNI_CAN_IMP_MESSAGE_TOO_LARGE : UNI_CAN_IMP_WRONG_ADDRESS;
    }

    uni_can_imp_session_t *session = uni_can_imp_find_free_session(ctx, msg->address_to);
    if (!session) {
        return UNI_CAN_IMP_NO_FREE_SESSION;
    }

    bool reserved_ids[8] = {false};
    uni_can_imp_session_t *sessions = uni_can_imp_sessions(ctx);
    for (uint16_t i = 0; i < ctx->max_sessions; i++) {
        if (uni_can_imp_session_is_tx(&sessions[i]) && sessions[i].peer_address == msg->address_to) {
            reserved_ids[sessions[i].session_id >> 1U] = true;
        }
    }

    uint8_t unique_id = 0;
    while (unique_id < 8U && reserved_ids[unique_id]) {
        unique_id++;
    }
    if (unique_id == 8U) {
        return UNI_CAN_IMP_NO_FREE_SESSION;
    }

    const uint32_t address_mask = session->address_mask;
    memset(session, 0, sizeof(*session));
    session->address_mask = address_mask;
    session->peer_address = msg->address_to;
    session->local_address = ctx->own_address;
    session->session_id = (uint8_t)((unique_id << 1U) | (ctx->own_address > msg->address_to ? 1U : 0U));
    session->mode = msg->address_to == UNI_CAN_IMP_DEVICE_ID_ALL ? UNI_CAN_IMP_MSG_MODE_WITHOUT_ACK : msg->mode;
    session->last_activity_ms = now_ms;

    const uni_can_imp_result_t copy_result = uni_can_imp_session_copy_payload(session, msg->data, msg->length);
    if (copy_result != UNI_CAN_IMP_OK) {
        uni_can_imp_session_release(session);
        return copy_result;
    }

    if (session->mode == UNI_CAN_IMP_MSG_MODE_WITHOUT_ACK) {
        session->state = UNI_CAN_IMP_SESSION_TX_WITHOUT_ACK;
        uni_can_imp_set_frame(ctx, output, session->peer_address, session->session_id, UNI_CAN_IMP_CMD_START_WA, NULL, 0);
    } else {
        session->state = UNI_CAN_IMP_SESSION_TX_WAIT_START_ACK;
        uni_can_imp_set_frame(ctx, output, session->peer_address, session->session_id, UNI_CAN_IMP_CMD_START, NULL, 0);
    }
    output->peer_address = session->peer_address;
    output->session_id = session->session_id;
    return UNI_CAN_IMP_OK;
}

uni_can_imp_result_t uni_can_imp_add_frame(uni_can_imp_ctx_t *ctx, const uni_can_message_t *frame,
                                           uint32_t now_ms, uni_can_imp_output_t *output) {
    if (!ctx || !ctx->sessions || !frame || !output) {
        return UNI_CAN_IMP_INVALID_ARGUMENT;
    }
    uni_can_imp_output_clear(output);

    if (frame->len > UNI_CAN_MESSAGE_MAXLEN) {
        return UNI_CAN_IMP_INVALID_FRAME;
    }

    const uint8_t address_to = uni_can_imp_canid_get_to(frame->id);
    const uint8_t address_from = uni_can_imp_canid_get_from(frame->id);
    const uint8_t session_id = uni_can_imp_canid_get_session_id(frame->id);
    const uni_can_imp_cmd_t command = uni_can_imp_canid_get_command(frame->id);

    if ((address_to != ctx->own_address && address_to != UNI_CAN_IMP_DEVICE_ID_ALL) ||
        !uni_can_imp_source_address_valid(address_from)) {
        return UNI_CAN_IMP_WRONG_ADDRESS;
    }
    if (command > UNI_CAN_IMP_CMD_ABORT_WA) {
        return UNI_CAN_IMP_INVALID_FRAME;
    }
    if (address_to == UNI_CAN_IMP_DEVICE_ID_ALL && !uni_can_imp_command_without_ack(command)) {
        return UNI_CAN_IMP_INVALID_FRAME;
    }

    uni_can_imp_session_t *session = uni_can_imp_find_session(ctx, address_from, address_to, session_id);
    if (!session && (command == UNI_CAN_IMP_CMD_START || command == UNI_CAN_IMP_CMD_START_WA)) {
        session = uni_can_imp_find_free_session(ctx, address_from);
        if (session) {
            const uint32_t address_mask = session->address_mask;
            memset(session, 0, sizeof(*session));
            session->address_mask = address_mask;
            session->state = UNI_CAN_IMP_SESSION_RX;
            session->mode = command == UNI_CAN_IMP_CMD_START ? UNI_CAN_IMP_MSG_MODE_WITH_ACK
                                                             : UNI_CAN_IMP_MSG_MODE_WITHOUT_ACK;
            session->peer_address = address_from;
            session->local_address = address_to;
            session->session_id = session_id;
            session->last_activity_ms = now_ms;
        }
    }

    if (!session) {
        if (!uni_can_imp_command_without_ack(command)) {
            uni_can_imp_emit_abort(ctx, output, address_from, session_id);
        }
        return UNI_CAN_IMP_NO_FREE_SESSION;
    }

    if (uni_can_imp_session_is_tx(session)) {
        return uni_can_imp_handle_tx_frame(ctx, session, command, now_ms, output);
    }
    return uni_can_imp_handle_rx_frame(ctx, session, frame, command, now_ms, output);
}

uni_can_imp_result_t uni_can_imp_process(uni_can_imp_ctx_t *ctx, uint32_t now_ms, uni_can_imp_output_t *output) {
    if (!ctx || !ctx->sessions || !output) {
        return UNI_CAN_IMP_INVALID_ARGUMENT;
    }
    uni_can_imp_output_clear(output);

    uni_can_imp_session_t *sessions = uni_can_imp_sessions(ctx);
    for (uint16_t i = 0; i < ctx->max_sessions; i++) {
        uni_can_imp_session_t *session = &sessions[i];
        if (session->state == UNI_CAN_IMP_SESSION_RX || session->state == UNI_CAN_IMP_SESSION_TX_WAIT_START_ACK ||
            session->state == UNI_CAN_IMP_SESSION_TX_WAIT_DATA_ACK ||
            session->state == UNI_CAN_IMP_SESSION_TX_WAIT_END_ACK) {
            if ((uint32_t)(now_ms - session->last_activity_ms) > ctx->session_timeout_ms) {
                uni_can_imp_set_event(output,
                                      session->state == UNI_CAN_IMP_SESSION_RX ? UNI_CAN_IMP_EVENT_RX_TIMEOUT
                                                                              : UNI_CAN_IMP_EVENT_TX_TIMEOUT,
                                      session);
                uni_can_imp_session_release(session);
                return UNI_CAN_IMP_OK;
            }
        }
    }

    for (uint16_t i = 0; i < ctx->max_sessions; i++) {
        uni_can_imp_session_t *session = &sessions[i];
        if (session->state != UNI_CAN_IMP_SESSION_TX_WITHOUT_ACK) {
            continue;
        }

        if (session->payload_offset < session->payload_size) {
            const uint16_t bytes_left = (uint16_t)(session->payload_size - session->payload_offset);
            const uint8_t length =
                (uint8_t)(bytes_left > UNI_CAN_MESSAGE_MAXLEN ? UNI_CAN_MESSAGE_MAXLEN : bytes_left);
            uni_can_imp_set_frame(ctx, output, session->peer_address, session->session_id, UNI_CAN_IMP_CMD_DATA_WA,
                                  session->payload + session->payload_offset, length);
            session->payload_offset = (uint16_t)(session->payload_offset + length);
            output->peer_address = session->peer_address;
            output->session_id = session->session_id;
            return UNI_CAN_IMP_OK;
        }

        uni_can_imp_set_frame(ctx, output, session->peer_address, session->session_id, UNI_CAN_IMP_CMD_END_WA, NULL, 0);
        uni_can_imp_set_event(output, UNI_CAN_IMP_EVENT_TX_FINISHED, session);
        uni_can_imp_session_release(session);
        return UNI_CAN_IMP_OK;
    }

    return UNI_CAN_IMP_OK;
}
