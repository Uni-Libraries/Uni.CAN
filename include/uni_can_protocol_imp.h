#pragma once

// IMP (Intermodule Protocol) C API

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "uni_can_message.h"

#define UNI_CAN_IMP_VERSION "1.0.0"

#define UNI_CAN_IMP_DEVICE_ID_ALL 31U
#define UNI_CAN_IMP_DEVICE_ID_MAX 31U
#define UNI_CAN_IMP_SESSION_ID_MAX 15U
#define UNI_CAN_IMP_DEFAULT_MAX_SESSIONS 8U
#define UNI_CAN_IMP_DEFAULT_MAX_MESSAGE_SIZE 200U
#define UNI_CAN_IMP_DEFAULT_SESSION_TIMEOUT_MS 1000U

typedef enum {
    UNI_CAN_IMP_MSG_MODE_WITH_ACK = 0,
    UNI_CAN_IMP_MSG_MODE_WITHOUT_ACK,
} uni_can_imp_msg_mode_t;

// Numeric values are part of the IMP CAN identifier layout.
typedef enum {
    UNI_CAN_IMP_CMD_START = 0,
    UNI_CAN_IMP_CMD_START_WA,
    UNI_CAN_IMP_CMD_ACK,
    UNI_CAN_IMP_CMD_NACK,
    UNI_CAN_IMP_CMD_DATA,
    UNI_CAN_IMP_CMD_DATA_WA,
    UNI_CAN_IMP_CMD_END,
    UNI_CAN_IMP_CMD_END_WA,
    UNI_CAN_IMP_CMD_ABORT_WA,
} uni_can_imp_cmd_t;

typedef enum {
    UNI_CAN_IMP_OK = 0,
    UNI_CAN_IMP_FAILURE,
    UNI_CAN_IMP_INVALID_ARGUMENT,
    UNI_CAN_IMP_WRONG_ADDRESS,
    UNI_CAN_IMP_INVALID_FRAME,
    UNI_CAN_IMP_NO_FREE_SESSION,
    UNI_CAN_IMP_CANT_ALLOCATE,
    UNI_CAN_IMP_MESSAGE_TOO_LARGE,
    UNI_CAN_IMP_UNEXPECTED_COMMAND,
    UNI_CAN_IMP_MESSAGE_REJECTED,
} uni_can_imp_result_t;

typedef enum {
    UNI_CAN_IMP_EVENT_NONE = 0,
    UNI_CAN_IMP_EVENT_TX_FINISHED,
    UNI_CAN_IMP_EVENT_TX_TIMEOUT,
    UNI_CAN_IMP_EVENT_TX_ABORTED,
    UNI_CAN_IMP_EVENT_RX_FINISHED,
    UNI_CAN_IMP_EVENT_RX_TIMEOUT,
    UNI_CAN_IMP_EVENT_RX_ABORTED,
} uni_can_imp_event_t;

typedef struct uni_can_imp_msg {
    uni_can_imp_msg_mode_t mode;
    uint8_t address_from;
    uint8_t address_to;
    uint8_t session_id;
    uint16_t length;
    uint8_t *data; // owned by this struct
} uni_can_imp_msg_t;

typedef bool (*uni_can_imp_message_validator_t)(const uni_can_imp_msg_t *msg, void *user_data);

typedef struct {
    uint8_t own_address;
    uint16_t max_sessions;
    uint16_t max_message_size;
    uint32_t session_timeout_ms;

    // Optional per-session peer masks. Missing entries default to all peers.
    const uint32_t *session_address_masks;
    size_t session_address_masks_count;

    // Optional whole-message validator, equivalent to bsusat_imp_msg_crc_is_ok().
    uni_can_imp_message_validator_t message_validator;
    void *user_data;
} uni_can_imp_config_t;

// Private session state is stored in ctx->sessions.
typedef struct {
    uint8_t own_address;
    uint16_t max_sessions;
    uint16_t max_message_size;
    uint32_t session_timeout_ms;
    uni_can_imp_message_validator_t message_validator;
    void *user_data;
    void *sessions;
} uni_can_imp_ctx_t;

typedef struct {
    bool frame_ready;
    uni_can_message_t frame;

    // Set for UNI_CAN_IMP_EVENT_RX_FINISHED. The caller owns the message.
    uni_can_imp_msg_t *received_message;

    uni_can_imp_event_t event;
    uint8_t peer_address;
    uint8_t session_id;
} uni_can_imp_output_t;

/**
 * @brief Returns a stable text representation of an IMP result code.
 *
 * @param value Result code to convert.
 * @return A pointer to a static null-terminated string.
 */
const char *uni_can_imp_result_to_string(uni_can_imp_result_t value);

/**
 * @brief Builds a 29-bit extended IMP CAN identifier.
 *
 * Values are masked to the widths defined by IMP: five address bits, four
 * session bits, and five command bits.
 *
 * @param address_from Source device address.
 * @param address_to Destination device address.
 * @param session_id Session identifier.
 * @param command IMP command encoded in the frame.
 * @return Encoded 29-bit CAN identifier.
 */
uint32_t uni_can_imp_canid_create(uint8_t address_from, uint8_t address_to, uint8_t session_id, uni_can_imp_cmd_t command);

/**
 * @brief Extracts the source device address from an IMP CAN identifier.
 *
 * @param can_id Encoded IMP CAN identifier.
 * @return Source device address in the range 0..31.
 */
uint8_t uni_can_imp_canid_get_from(uint32_t can_id);

/**
 * @brief Extracts the destination device address from an IMP CAN identifier.
 *
 * @param can_id Encoded IMP CAN identifier.
 * @return Destination device address in the range 0..31.
 */
uint8_t uni_can_imp_canid_get_to(uint32_t can_id);

/**
 * @brief Extracts the session identifier from an IMP CAN identifier.
 *
 * @param can_id Encoded IMP CAN identifier.
 * @return Session identifier in the range 0..15.
 */
uint8_t uni_can_imp_canid_get_session_id(uint32_t can_id);

/**
 * @brief Extracts the command from an IMP CAN identifier.
 *
 * @param can_id Encoded IMP CAN identifier.
 * @return Command value encoded in the identifier.
 */
uni_can_imp_cmd_t uni_can_imp_canid_get_command(uint32_t can_id);

/**
 * @brief Allocates an IMP message and copies its payload.
 *
 * Broadcast messages are always converted to
 * UNI_CAN_IMP_MSG_MODE_WITHOUT_ACK as required by IMP.
 *
 * @param mode Requested transfer acknowledgement mode.
 * @param address_from Source device address; broadcast is not valid here.
 * @param address_to Destination device address or UNI_CAN_IMP_DEVICE_ID_ALL.
 * @param data Payload bytes. May be NULL only when @p length is zero.
 * @param length Payload length in bytes.
 * @return Newly allocated message, or NULL for invalid arguments or allocation
 *         failure. Free it with uni_can_imp_msg_free().
 */
uni_can_imp_msg_t *uni_can_imp_msg_create(uni_can_imp_msg_mode_t mode, uint8_t address_from, uint8_t address_to,
                                           const uint8_t *data, uint16_t length);

/**
 * @brief Frees an IMP message and its owned payload.
 *
 * @param msg Message returned by uni_can_imp_msg_create() or by an IMP output.
 *            NULL is accepted.
 */
void uni_can_imp_msg_free(uni_can_imp_msg_t *msg);

/**
 * @brief Initializes an IMP protocol context.
 *
 * The function allocates private session storage and copies scalar
 * configuration values. The address-mask array is consumed during this call
 * and may be released afterwards. A successfully initialized context must not
 * be copied and must be released with uni_can_imp_deinit().
 *
 * @param ctx Uninitialized context to configure.
 * @param cfg Protocol configuration.
 * @return true on success; false for invalid arguments or allocation failure.
 */
bool uni_can_imp_init(uni_can_imp_ctx_t *ctx, const uni_can_imp_config_t *cfg);

/**
 * @brief Releases all sessions and payloads owned by an IMP context.
 *
 * @param ctx Initialized context. NULL is accepted. The context is zeroed.
 */
void uni_can_imp_deinit(uni_can_imp_ctx_t *ctx);

/**
 * @brief Starts an outgoing IMP transfer.
 *
 * The payload is copied into the context, so @p msg may be released after the
 * call. On success, @p output contains the START or START_WA frame that the
 * caller must send. The session identifier is assigned automatically.
 *
 * @param ctx Initialized protocol context whose address matches the source.
 * @param msg Message to transfer.
 * @param now_ms Current monotonic time in milliseconds.
 * @param output Receives the first frame and session metadata. Existing output
 *               contents are cleared without freeing a received message.
 * @return UNI_CAN_IMP_OK on success, otherwise an IMP error code.
 */
uni_can_imp_result_t uni_can_imp_start_send(uni_can_imp_ctx_t *ctx, const uni_can_imp_msg_t *msg,
                                             uint32_t now_ms, uni_can_imp_output_t *output);

/**
 * @brief Processes one received IMP CAN frame.
 *
 * The function updates the matching session, advances acknowledged transfers,
 * and may produce a response frame, a transfer event, and/or a completed
 * message. When output->received_message is set, ownership passes to the
 * caller, which must call uni_can_imp_msg_free().
 *
 * @param ctx Initialized protocol context.
 * @param frame Received CAN frame.
 * @param now_ms Current monotonic time in milliseconds.
 * @param output Receives protocol output. Existing contents are cleared
 *               without freeing a received message.
 * @return UNI_CAN_IMP_OK when the frame was handled, otherwise an IMP error
 *         code. Output may still contain a protocol response on error.
 */
uni_can_imp_result_t uni_can_imp_add_frame(uni_can_imp_ctx_t *ctx, const uni_can_message_t *frame,
                                            uint32_t now_ms, uni_can_imp_output_t *output);

/**
 * @brief Advances pending transfers and checks session timeouts.
 *
 * Each call returns at most one DATA_WA/END_WA frame or one timeout event.
 * Call this function repeatedly to drain unacknowledged transfers. Transfers
 * with acknowledgements advance through uni_can_imp_add_frame().
 *
 * @param ctx Initialized protocol context.
 * @param now_ms Current monotonic time in milliseconds.
 * @param output Receives the next frame or event. Existing contents are
 *               cleared without freeing a received message.
 * @return UNI_CAN_IMP_OK on success, or UNI_CAN_IMP_INVALID_ARGUMENT.
 */
uni_can_imp_result_t uni_can_imp_process(uni_can_imp_ctx_t *ctx, uint32_t now_ms, uni_can_imp_output_t *output);

/**
 * @brief Clears an IMP output structure.
 *
 * This function does not free output->received_message. The caller must retain
 * and free that pointer before clearing or reusing the output structure.
 *
 * @param output Output structure to clear. NULL is accepted.
 */
void uni_can_imp_output_clear(uni_can_imp_output_t *output);

#if defined(__cplusplus)
}
#endif
