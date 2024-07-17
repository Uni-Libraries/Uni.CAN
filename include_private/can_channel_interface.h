#pragma once

// stdlib
#include <utility>

// auris.can
#include "uni_can_message.h"

namespace Auris::CAN {
    class ICanChannel {
      public:
        virtual ~ICanChannel() = default;

        virtual bool Init() = 0;
        virtual bool DeInit() = 0;

        virtual bool Open() = 0;
        virtual bool Close() = 0;

        virtual bool ReceiveMessage(uni_can_message_t &msg) = 0;
        virtual bool TransmitMessage(const uni_can_message_t &msg) = 0;
    };
}