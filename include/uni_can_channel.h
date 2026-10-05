#pragma once

// uni.can
#include "uni_can_message.h"

#if defined(__cplusplus)
extern "C" {
#endif

//
// Includes
//

// stdlib
#include <stdbool.h>
#include <stdint.h>



//
// Typedefs
//

typedef void (* uni_can_channel_receive_handler_f)(void* channel, void* cookie);


//
// Typedefs/Backend
//

/**
 * Operations of a channel, filled in by its backend. An operation the adapter of the backend
 * has no means for stays null; the function of the same name then returns false.
 * Every operation gets the channel it was called for, the `void *channel` of the functions below.
 */
typedef struct {
    bool (*init)(void *channel);

    bool (*open)(void *channel);

    bool (*close)(void *channel);

    bool (*destroy)(void *channel);

    /**
     * Take a received message as an object on the heap. A backend that has `receive_to` may
     * leave this null: uni_can_channel_receive() then allocates the message itself.
     */
    uni_can_message_t* (*receive)(void *channel);

    /**
     * Take a received message into memory of the caller, without using the heap
     */
    bool (*receive_to)(void *channel, uni_can_message_t *msg);

    bool (*transmit)(void *channel, const uni_can_message_t *msg);

    bool (*transmit_idle)(void *channel);

    bool (*transmit_abort)(void *channel);

    bool (*set_receive_handler)(void *channel, uni_can_channel_receive_handler_f func, void *cookie);

    bool (*filter_add)(void *channel, uint32_t id, uint32_t mask);
} uni_can_channel_ops_t;

/**
 * What every channel starts with. A backend puts it first in the object of its channel; a
 * pointer to that object is what the functions below take as `channel`.
 */
typedef struct {
    const uni_can_channel_ops_t *ops;
} uni_can_channel_base_t;



//
// Functions
//

bool uni_can_channel_init(void *channel);

/**
 * Go on the bus
 */
bool uni_can_channel_open(void *channel);

/**
 * Leave the bus
 */
bool uni_can_channel_close(void *channel);

/**
 * Close the channel and release what it holds. A channel that came from the factory is gone
 * after this call; one that lives in memory of the application can be set up again.
 */
bool uni_can_channel_destroy(void *channel);

/**
 * Take the oldest received message
 * @return the message, to be released with uni_can_message_free(); null when there is none
 */
uni_can_message_t* uni_can_channel_receive(void *channel);

/**
 * Take the oldest received message without using the heap
 * @param msg receives the message
 * @return false when there is none
 */
bool uni_can_channel_receive_to(void *channel, uni_can_message_t *msg);

/**
 * Hand a message to the adapter for transmission. The call does not wait for the message to
 * leave.
 * @return false when the adapter did not take it
 */
bool uni_can_channel_transmit(void *channel, const uni_can_message_t *msg);

/**
 * Check that the adapter holds no message that still waits to be sent
 * @return true when every message handed to uni_can_channel_transmit() has left or was dropped;
 *         false also when the backend cannot tell
 */
bool uni_can_channel_transmit_idle(void *channel);

/**
 * Drop the messages that still wait in the adapter to be sent, e.g. because nobody
 * acknowledges them
 */
bool uni_can_channel_transmit_abort(void *channel);

/**
 * Set the function that is called when a message has arrived
 */
bool uni_can_channel_set_receive_handler(void* channel, uni_can_channel_receive_handler_f func, void* cookie);

/**
 * Accept the messages whose identifier equals `id` in the bits that are set in `mask`, in
 * addition to what earlier calls accept. Identifiers are compared as 29-bit ones.
 * Without a filter a channel accepts nothing or everything, depending on its adapter.
 * @return false when the adapter has no room for another filter, or no filters at all
 */
bool uni_can_channel_filter_add(void *channel, uint32_t id, uint32_t mask);

#if defined(__cplusplus)
}
#endif
