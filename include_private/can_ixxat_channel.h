#pragma once

#if defined(_MSC_VER)

// stdlib
#include <deque>
#include <memory>
#include <thread>

// IXXAT VCI SDK
#include <vcisdk.h>

// Uni.CAN
#include "can_socketcan_channel.h"
#include "common_queue.h"

namespace Uni::CAN {
    class CanChannelIxxat : public ICanChannel {
        //
        // ICanChannel
        //
    public:
        ~CanChannelIxxat() override;

        bool Init() override;

        bool DeInit() override;

        bool Open() override;

        bool Close() override;

        bool ReceiveMessage(uni_can_message_t &msg) override;

        bool TransmitMessage(const uni_can_message_t &msg) override;

        //
        // Ctor
        //
    protected:
        friend class CanProviderIxxat;

        explicit CanChannelIxxat(uni_can_devinfo_t *devInfo, size_t channelIdx, uint32_t baudrate);

        //
        // Init
        //
    private:
        bool InitLine();

        void Deinit();

        //
        // Thread
        //
    private:
        void threadProc();

        bool threadStop();

        bool threadStart();

    private:
        bool _threadTerminate = false;
        std::thread _thread;

        //
        // Read
        //
    private:
        bool readProcess();

    private:
        SharedQueue<uni_can_message_t> _read_queue;

        //
        // Consts
        //
    private:
        static constexpr uint16_t wRxFifoSize = 1024;
        static constexpr uint16_t wRxThreshold = 1;
        static constexpr uint16_t wTxFifoSize = 128;
        static constexpr uint16_t wTxThreshold = 1;
        static constexpr uint16_t _const_read_timeout_ms = 100;

        //
        // Vars
        //
    private:
        uni_can_devinfo_t _dev_info{};

        IVciDeviceManager *_device_mgr = nullptr;
        IVciDevice *_device = nullptr;
        IBalObject *_device_bal = nullptr;

        size_t _channel_idx;

        ICanSocket *_can_socket = nullptr;
        ::ICanChannel *_can_channel = nullptr;
        ICanControl2 *_can_control = nullptr;

        PFIFOREADER _can_reader = nullptr;
        HANDLE _can_reader_event = nullptr;

        PFIFOWRITER _can_writer = nullptr;

        uint32_t _can_baudrate = 0;
    };
} // namespace Uni::CAN

#endif
