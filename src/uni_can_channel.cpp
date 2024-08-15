//
// Includes
//

// uni.can
#include "uni_can_channel.h"

#include "can_channel_interface.h"


//
//
//

bool uni_can_channel_init(void *channel) {
    if (channel) {
        return static_cast<Uni::CAN::ICanChannel *>(channel)->Init();
    }
    return false;
}

bool uni_can_channel_open(void *channel) {
    if (channel) {
        return static_cast<Uni::CAN::ICanChannel *>(channel)->Open();
    }
    return false;
}

bool uni_can_channel_close(void *channel) {
    if (channel) {
        return static_cast<Uni::CAN::ICanChannel *>(channel)->Close();
    }
    return false;
}

bool uni_can_channel_destroy(void *channel) {
    if (channel) {
        delete static_cast<Uni::CAN::ICanChannel *>(channel);
        return true;
    }
    return false;
}

uni_can_message_t* uni_can_channel_receive(void *channel) {
    uni_can_message_t* result = nullptr;

    if (channel) {
        result = static_cast<Uni::CAN::ICanChannel *>(channel)->ReceiveMessage();
    }
    return result;
}

bool uni_can_channel_transmit(void *channel, const uni_can_message_t *msg) {
    if (channel) {
        return static_cast<Uni::CAN::ICanChannel *>(channel)->TransmitMessage(*msg);
    }
    return false;
}
