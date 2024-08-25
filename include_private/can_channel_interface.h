#pragma once

// stdlib
#include <utility>

// Uni.CAN
#include "uni_can_channel.h"
#include "uni_can_message.h"

namespace Uni::CAN {
    class ICanChannel {
    public:
        virtual ~ICanChannel() = default;

        virtual bool Init() = 0;

        virtual bool DeInit() = 0;

        virtual bool Open() = 0;

        virtual bool Close() = 0;

        [[nodiscard]] virtual uni_can_message_t* ReceiveMessage() = 0;

        virtual bool TransmitMessage(const uni_can_message_t &msg) = 0;

        virtual void ReceiveHandlerSet(uni_can_channel_receive_handler_f func, void* cookie) = 0;
    };
}
