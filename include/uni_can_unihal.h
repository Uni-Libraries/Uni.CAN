#pragma once

// Backend for the CAN driver of Uni.HAL: a channel on a CAN peripheral of a microcontroller.
//
// The channel lives in memory of the application and does not use the heap. It is not made
// by the factory: the application describes the peripheral as an uni_hal_can_context_t, sets
// the channel up with uni_can_unihal_channel_setup() and then uses the uni_can_channel_
// functions on it.

#if defined(__cplusplus)
extern "C" {
#endif

//
// Includes
//

// stdlib
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Uni.HAL
#include <uni_hal.h>

// uni.can
#include "uni_can_channel.h"



//
// Defines
//

/**
 * Filters one channel can have
 */
#define UNI_CAN_UNIHAL_FILTERS_MAX (14U)



//
// Typedefs
//

typedef struct {
    /**
     * CAN peripheral. uni_can_channel_init() initialises it when the application has not.
     */
    uni_hal_can_context_t *can;

    /**
     * Receive FIFO of the peripheral the filters of this channel feed
     */
    uint32_t filter_fifo;

    /**
     * First filter slot of the peripheral that belongs to this channel, as
     * uni_hal_can_set_filter() numbers them, and the number of slots from there on. Not more
     * than UNI_CAN_UNIHAL_FILTERS_MAX of them are used.
     */
    uint32_t filter_slot_first;
    uint32_t filter_slot_count;
} uni_can_unihal_config_t;

typedef struct {
    // first: the uni_can_channel_ functions find the operations here
    uni_can_channel_base_t base;

    uni_can_unihal_config_t config;

    /** Filters that are set, in the order of their slots */
    uint32_t filter_id[UNI_CAN_UNIHAL_FILTERS_MAX];
    uint32_t filter_mask[UNI_CAN_UNIHAL_FILTERS_MAX];
    size_t filter_count;

    bool opened;
} uni_can_unihal_channel_t;



//
// Functions
//

/**
 * Make a channel of a CAN peripheral of Uni.HAL. Nothing happens to the peripheral yet:
 * uni_can_channel_init() brings it up, uni_can_channel_open() takes it on the bus.
 *
 * What the channel does:
 * - open: uni_hal_can_start(). It fails while the bus is not idle and can be tried again.
 * - receive: data frames with up to 8 bytes; remote frames and longer CAN FD frames are
 *   dropped. Without a filter nothing is received. Messages carry no time.
 * - transmit: one frame into a free TX mailbox, without waiting for it to leave.
 * - A receive handler cannot be set: the application polls.
 *
 * @param channel memory for the channel
 * @param config peripheral and its filters; copied
 * @return false when an argument is missing
 */
bool uni_can_unihal_channel_setup(uni_can_unihal_channel_t *channel, const uni_can_unihal_config_t *config);

#if defined(__cplusplus)
}
#endif
