//
// Includes
//

// stdlib
#include <stddef.h>

// uni.can
#include "uni_can_channel.h"



//
// Private
//

static const uni_can_channel_ops_t *_uni_can_channel_ops(void *channel) {
    const uni_can_channel_ops_t *result = NULL;
    if (channel != NULL) {
        result = ((const uni_can_channel_base_t *)channel)->ops;
    }
    return result;
}



//
// Functions
//

bool uni_can_channel_init(void *channel) {
    const uni_can_channel_ops_t *ops = _uni_can_channel_ops(channel);
    return (ops != NULL) && (ops->init != NULL) && ops->init(channel);
}

bool uni_can_channel_open(void *channel) {
    const uni_can_channel_ops_t *ops = _uni_can_channel_ops(channel);
    return (ops != NULL) && (ops->open != NULL) && ops->open(channel);
}

bool uni_can_channel_close(void *channel) {
    const uni_can_channel_ops_t *ops = _uni_can_channel_ops(channel);
    return (ops != NULL) && (ops->close != NULL) && ops->close(channel);
}

bool uni_can_channel_destroy(void *channel) {
    const uni_can_channel_ops_t *ops = _uni_can_channel_ops(channel);
    return (ops != NULL) && (ops->destroy != NULL) && ops->destroy(channel);
}

uni_can_message_t *uni_can_channel_receive(void *channel) {
    uni_can_message_t *result = NULL;

    const uni_can_channel_ops_t *ops = _uni_can_channel_ops(channel);
    if (ops != NULL) {
        if (ops->receive != NULL) {
            result = ops->receive(channel);
        }
        else if (ops->receive_to != NULL) {
            result = uni_can_message_create();
            if ((result != NULL) && !ops->receive_to(channel, result)) {
                uni_can_message_free(result);
                result = NULL;
            }
        }
        else {
            // the backend does not receive
        }
    }

    return result;
}

bool uni_can_channel_receive_to(void *channel, uni_can_message_t *msg) {
    bool result = false;

    // No help from the heap here: a program that takes its messages this way may have none.
    const uni_can_channel_ops_t *ops = _uni_can_channel_ops(channel);
    if ((ops != NULL) && (ops->receive_to != NULL) && (msg != NULL)) {
        result = ops->receive_to(channel, msg);
    }

    return result;
}

bool uni_can_channel_transmit(void *channel, const uni_can_message_t *msg) {
    const uni_can_channel_ops_t *ops = _uni_can_channel_ops(channel);
    return (ops != NULL) && (ops->transmit != NULL) && (msg != NULL) && ops->transmit(channel, msg);
}

bool uni_can_channel_transmit_idle(void *channel) {
    const uni_can_channel_ops_t *ops = _uni_can_channel_ops(channel);
    return (ops != NULL) && (ops->transmit_idle != NULL) && ops->transmit_idle(channel);
}

bool uni_can_channel_transmit_abort(void *channel) {
    const uni_can_channel_ops_t *ops = _uni_can_channel_ops(channel);
    return (ops != NULL) && (ops->transmit_abort != NULL) && ops->transmit_abort(channel);
}

bool uni_can_channel_set_receive_handler(void *channel, uni_can_channel_receive_handler_f func, void *cookie) {
    const uni_can_channel_ops_t *ops = _uni_can_channel_ops(channel);
    return (ops != NULL) && (ops->set_receive_handler != NULL) && ops->set_receive_handler(channel, func, cookie);
}

bool uni_can_channel_filter_add(void *channel, uint32_t id, uint32_t mask) {
    const uni_can_channel_ops_t *ops = _uni_can_channel_ops(channel);
    return (ops != NULL) && (ops->filter_add != NULL) && ops->filter_add(channel, id, mask);
}
