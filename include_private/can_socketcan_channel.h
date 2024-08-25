#pragma once

// stdlib
#include <cstdint>
#include <memory>
#include <thread>

// Uni.CAN
#include "uni_can_devinfo.h"
#include "can_channel_interface.h"
#include "common_queue.h"


//
// Public
//

namespace Uni::CAN {
    class CanChannelSocketcan : public ICanChannel {
    public:
        ~CanChannelSocketcan() override;

        bool Init() override;

        bool DeInit() override;

        bool Open() override;

        bool Close() override;

        bool TransmitMessage(const uni_can_message_t &msg) override;

        //
        // Receive
        //
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
        int _thread_fd = -1;
        std::thread _thread;


    protected:
        friend class CanProviderSocketcan;

        explicit CanChannelSocketcan(const uni_can_devinfo_t *devInfo, int channel_idx, uint32_t baudrate);

    private:
        uni_can_devinfo_t _dev_info{};
        uint32_t _can_baudrate{0};
        uint32_t _can_restart{100};

        int _fd = -1;
    };
} // namespace Uni::CAN
