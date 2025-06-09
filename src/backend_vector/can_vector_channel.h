#pragma once

#if defined(_WIN32)

//
// Includes
//

// uni.can
#include "can_channel_base.h"



//
// Class
//

namespace Uni::CAN {
    class CanChannelVector : public CanChannelBase {
        //ctor
    public:
        explicit CanChannelVector(uni_can_devinfo_t *devInfo, uint32_t baudrate);
        ~CanChannelVector() override;

        //ICanChannel
    public:
        bool Init() override;
        bool DeInit() override;
        bool Open() override;
        bool Close() override;
        bool TransmitMessage(const uni_can_message_t &msg) override;

        // device
    private:
        long _device_handle{};

        // thread
    private:
        void threadProc() override;
        void threadProcReceive(void* event);
    private:
        static constexpr auto _threadWaitTime = 50;
    };
} // namespace Uni::CAN

#endif
