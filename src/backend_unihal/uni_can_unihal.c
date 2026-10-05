//
// Includes
//

// stdlib
#include <string.h>

// uni.can
#include "uni_can_unihal.h"



//
// Defines
//

/**
 * TX mailboxes of a CAN peripheral, as uni_hal_can_transmit_free() counts them
 */
#define UNI_CAN_UNIHAL_TX_MAILBOXES (3U)

/**
 * uni_hal_can_set_filter() takes identifier and mask in the layout of the bxCAN filter
 * registers: an extended identifier starts at bit 3
 */
#define UNI_CAN_UNIHAL_FILTER_EXT_ID_POS (3U)

#define UNI_CAN_UNIHAL_EXT_ID_MASK (0x1FFFFFFFU)



//
// Private
//

static uni_can_unihal_channel_t *_uni_can_unihal(void *channel) {
    return (uni_can_unihal_channel_t *)channel;
}

static bool _uni_can_unihal_init(void *channel) {
    uni_can_unihal_channel_t *self = _uni_can_unihal(channel);
    return uni_hal_can_is_inited(self->config.can) || uni_hal_can_init(self->config.can);
}

static bool _uni_can_unihal_open(void *channel) {
    uni_can_unihal_channel_t *self = _uni_can_unihal(channel);
    self->opened = uni_hal_can_start(self->config.can);
    return self->opened;
}

static bool _uni_can_unihal_close(void *channel) {
    uni_can_unihal_channel_t *self = _uni_can_unihal(channel);
    self->opened = false;
    return uni_hal_can_stop(self->config.can);
}

static bool _uni_can_unihal_destroy(void *channel) {
    uni_can_unihal_channel_t *self = _uni_can_unihal(channel);

    // the peripheral may never have been brought up: there is nothing to stop then
    if (uni_hal_can_is_inited(self->config.can)) {
        (void)_uni_can_unihal_close(channel);
    }

    // the object is of no use any more, until it is set up again
    self->base.ops = NULL;
    return true;
}

static bool _uni_can_unihal_receive_to(void *channel, uni_can_message_t *msg) {
    uni_can_unihal_channel_t *self = _uni_can_unihal(channel);

    uni_hal_can_msg_t rx = {0};
    while (uni_hal_can_receive(self->config.can, &rx, 0U)) {
        // A remote frame asks for data and carries none; a CAN FD frame can carry more than a
        // message of this library holds.
        if (rx.remote || (rx.dlc > UNI_CAN_MESSAGE_MAXLEN)) {
            continue;
        }

        *msg = (uni_can_message_t){0};
        msg->id = rx.id;
        msg->flags = rx.standard_id ? UNI_CAN_MSG_FLAG_STD_ID : UNI_CAN_MSG_FLAG_EXT_ID;
        msg->len = rx.dlc;
        (void)memcpy(msg->data.u8, rx.data, rx.dlc);
        return true;
    }

    return false;
}

static bool _uni_can_unihal_transmit(void *channel, const uni_can_message_t *msg) {
    uni_can_unihal_channel_t *self = _uni_can_unihal(channel);

    // the data of a transport protocol message is not in the message itself
    if ((msg->len > UNI_CAN_MESSAGE_MAXLEN) || ((msg->flags & UNI_CAN_MSG_FLAG_TP) != 0)) {
        return false;
    }

    uni_hal_can_msg_t tx = {0};
    tx.id = msg->id;
    tx.dlc = (uint8_t)msg->len;
    tx.standard_id = ((msg->flags & UNI_CAN_MSG_FLAG_STD_ID) != 0) && ((msg->flags & UNI_CAN_MSG_FLAG_EXT_ID) == 0);
    (void)memcpy(tx.data, msg->data.u8, msg->len);

    return uni_hal_can_transmit_nowait(self->config.can, &tx);
}

static bool _uni_can_unihal_transmit_idle(void *channel) {
    uni_can_unihal_channel_t *self = _uni_can_unihal(channel);
    return uni_hal_can_transmit_free(self->config.can) == UNI_CAN_UNIHAL_TX_MAILBOXES;
}

static bool _uni_can_unihal_transmit_abort(void *channel) {
    uni_can_unihal_channel_t *self = _uni_can_unihal(channel);
    return uni_hal_can_transmit_abort(self->config.can);
}

static bool _uni_can_unihal_filter_add(void *channel, uint32_t id, uint32_t mask) {
    uni_can_unihal_channel_t *self = _uni_can_unihal(channel);

    id &= UNI_CAN_UNIHAL_EXT_ID_MASK;
    mask &= UNI_CAN_UNIHAL_EXT_ID_MASK;

    // a filter that is there already needs no second slot
    for (size_t idx = 0U; idx < self->filter_count; idx++) {
        if ((self->filter_id[idx] == id) && (self->filter_mask[idx] == mask)) {
            return true;
        }
    }

    size_t slots = self->config.filter_slot_count;
    if (slots > UNI_CAN_UNIHAL_FILTERS_MAX) {
        slots = UNI_CAN_UNIHAL_FILTERS_MAX;
    }
    if (self->filter_count >= slots) {
        return false;
    }

    // The kind of the frame is left out of the comparison: a standard frame whose identifier
    // happens to match in that layout passes as well.
    const uint32_t slot = self->config.filter_slot_first + (uint32_t)self->filter_count;
    if (!uni_hal_can_set_filter(self->config.can, self->config.filter_fifo, slot,
                                id << UNI_CAN_UNIHAL_FILTER_EXT_ID_POS, mask << UNI_CAN_UNIHAL_FILTER_EXT_ID_POS)) {
        return false;
    }

    self->filter_id[self->filter_count] = id;
    self->filter_mask[self->filter_count] = mask;
    self->filter_count++;
    return true;
}

static const uni_can_channel_ops_t g_uni_can_unihal_ops = {
    .init = _uni_can_unihal_init,
    .open = _uni_can_unihal_open,
    .close = _uni_can_unihal_close,
    .destroy = _uni_can_unihal_destroy,
    .receive = NULL,
    .receive_to = _uni_can_unihal_receive_to,
    .transmit = _uni_can_unihal_transmit,
    .transmit_idle = _uni_can_unihal_transmit_idle,
    .transmit_abort = _uni_can_unihal_transmit_abort,
    // Uni.HAL does not tell when a frame has arrived
    .set_receive_handler = NULL,
    .filter_add = _uni_can_unihal_filter_add,
};



//
// Functions
//

bool uni_can_unihal_channel_setup(uni_can_unihal_channel_t *channel, const uni_can_unihal_config_t *config) {
    bool result = false;

    if ((channel != NULL) && (config != NULL) && (config->can != NULL)) {
        (void)memset(channel, 0, sizeof(*channel));
        channel->base.ops = &g_uni_can_unihal_ops;
        channel->config = *config;
        result = true;
    }

    return result;
}
