#pragma once

//
// Includes
//

// stdlib
#include <chrono>
#include <thread>

// uni.can
#include "can_channel_interface.h"
#include "common_queue.h"
#include "uni_can_devinfo.h"


//
// Class
//

namespace Uni::CAN
{
    class CanChannelBase: public ICanChannel
    {
        //
        // Info
        //
    protected:
        uint8_t _info_chidx = -1;
        uni_can_devinfo_t _info_dev{};
        uint32_t _info_baudrate = 0;

        //
        // Receive
        //
    public:
        [[nodiscard]] uni_can_message_t* ReceiveMessage() override;
        void ReceiveHandlerSet(uni_can_channel_receive_handler_f func, void* cookie) override;
    protected:
        uni_can_channel_receive_handler_f m_receive_func{};
        void* m_receive_cookie{};
        SharedQueue<uni_can_message_t*> m_receive_queue;

        //
        // Thread
        //
    protected:
        virtual void threadProc() = 0;
        virtual bool threadStop();
        bool threadStart();
    protected:
        bool _thread_abort{};
    private:
        std::thread _thread;

        //
        // Time
        //
    protected:
        std::chrono::steady_clock::time_point _time_start{};
    };
}