#pragma once

// stdlib
#include <cstdint>
#include <memory>

// uni.CAN
#include "uni_can_channel.h"
#include "uni_can_devinfo.h"
#include "can_channel_interface.h"
#include "common_queue.h"

namespace Uni::CAN {
    class CanChannelChai : public ICanChannel {
    public:
        ~CanChannelChai() override;

        bool Init() override;

        bool DeInit() override;

        bool Open() override;

        bool Close() override;

        // Receive
    public:
        [[nodiscard]] uni_can_message_t* ReceiveMessage() override;

        void ReceiveHandlerSet(uni_can_channel_receive_handler_f func, void* cookie) override;
    private:
        bool receiveMessage();
    private:
        uni_can_channel_receive_handler_f m_receive_func{};
        void* m_receive_cookie{};
        SharedQueue<uni_can_message_t*> m_receive_queue;

        //
        // Thread
        //
    private:
        void threadProc();
        bool threadStop();
        bool threadStart();

    private:
        bool _thread_abort;
        std::thread _thread;


        // Transmit
    public:
        bool TransmitMessage(const uni_can_message_t &msg) override;

    protected:
        friend class CanProviderChai;

        explicit CanChannelChai(uni_can_devinfo_t *devInfo, size_t channelIdx, uint32_t baudrate);

    private:
        uint8_t _channel_num = -1;
        uni_can_devinfo_t _dev_info;
        uint32_t _can_baudrate = 0;
    };
} // namespace Uni::CAN
