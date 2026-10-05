//
// Includes
//

// stdlib
#include <memory>

// uni.can
#include "can_channel_adapter.h"



//
// Private
//

namespace {
    /**
     * Channel of a backend written in C++, as the C functions see it
     */
    struct CanChannelAdapter {
        // first: the C functions find the operations here
        uni_can_channel_base_t base{};
        std::unique_ptr<Uni::CAN::ICanChannel> channel{};

        uni_can_channel_receive_handler_f receive_func{};
        void* receive_cookie{};
    };

    CanChannelAdapter* adapter(void* channel) {
        return static_cast<CanChannelAdapter*>(channel);
    }

    bool adapter_init(void* channel) {
        return adapter(channel)->channel->Init();
    }

    bool adapter_open(void* channel) {
        return adapter(channel)->channel->Open();
    }

    bool adapter_close(void* channel) {
        return adapter(channel)->channel->Close();
    }

    bool adapter_destroy(void* channel) {
        delete adapter(channel);
        return true;
    }

    uni_can_message_t* adapter_receive(void* channel) {
        return adapter(channel)->channel->ReceiveMessage();
    }

    bool adapter_receive_to(void* channel, uni_can_message_t* msg) {
        uni_can_message_t* received = adapter_receive(channel);
        if (!received) {
            return false;
        }

        // The data of a transport protocol message is on the heap and goes over to the caller
        // with the copy: only the object it came in is released here.
        *msg = *received;
        received->flags = static_cast<uni_can_message_flags_t>(received->flags & ~UNI_CAN_MSG_FLAG_TP);
        uni_can_message_free(received);
        return true;
    }

    bool adapter_transmit(void* channel, const uni_can_message_t* msg) {
        return adapter(channel)->channel->TransmitMessage(*msg);
    }

    /**
     * The backend reports its own object; the application knows the channel by the adapter
     */
    void adapter_receive_handler(void*, void* cookie) {
        auto* self = adapter(cookie);
        const auto func = self->receive_func;
        if (func) {
            func(self, self->receive_cookie);
        }
    }

    bool adapter_set_receive_handler(void* channel, uni_can_channel_receive_handler_f func, void* cookie) {
        auto* self = adapter(channel);
        self->receive_cookie = cookie;
        self->receive_func = func;
        return true;
    }

    // The adapters behind these backends are driven by their vendor libraries: nothing tells
    // whether a message has left, none can be taken back, and filtering is left to the application.
    const uni_can_channel_ops_t adapter_ops = {
        &adapter_init,                // init
        &adapter_open,                // open
        &adapter_close,               // close
        &adapter_destroy,             // destroy
        &adapter_receive,             // receive
        &adapter_receive_to,          // receive_to
        &adapter_transmit,            // transmit
        nullptr,                      // transmit_idle
        nullptr,                      // transmit_abort
        &adapter_set_receive_handler, // set_receive_handler
        nullptr,                      // filter_add
    };
}



//
// Implementation
//

namespace Uni::CAN {
    void* CanChannelAdapt(ICanChannel* channel) {
        if (!channel) {
            return nullptr;
        }

        auto* result = new CanChannelAdapter();
        result->base.ops = &adapter_ops;
        result->channel.reset(channel);
        channel->ReceiveHandlerSet(&adapter_receive_handler, result);
        return result;
    }
}
