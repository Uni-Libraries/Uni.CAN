#pragma once

#if defined(_WIN32)

// stdlib
#include <memory>
#include <thread>

// IXXAT VCI SDK
#include <vcisdk.h>

// Uni.CAN
#include "can_socketcan_channel.h"
namespace Uni::CAN {
    class CanChannelIxxat : public CanChannelBase {
        // Ctor
    public:
        explicit CanChannelIxxat(uni_can_devinfo_t *devInfo, size_t channelIdx, uint32_t baudrate);
        ~CanChannelIxxat() override;

        // ICanChannel
    public:
        bool Init() override;
        bool DeInit() override;
        bool Open() override;
        bool Close() override;
        bool TransmitMessage(const uni_can_message_t &msg) override;

        // thread
    private:
        void threadProc() override;
        bool threadProcReceive();

        //
        // Consts
        //
    private:
        static constexpr uint16_t wRxFifoSize = 1024;
        static constexpr uint16_t wRxThreshold = 1;
        static constexpr uint16_t wTxFifoSize = 128;
        static constexpr uint16_t wTxThreshold = 1;
        static constexpr uint16_t _const_read_timeout_ms = 100;

        // ixxat specific
    private:
        IVciDeviceManager *_device_mgr = nullptr;
        IVciDevice *_device = nullptr;
        IBalObject *_device_bal = nullptr;
        ICanSocket *_can_socket = nullptr;
        ::ICanChannel *_can_channel = nullptr;
        ICanControl2 *_can_control = nullptr;
        PFIFOREADER _can_reader = nullptr;
        HANDLE _can_reader_event = nullptr;
        CANCAPABILITIES _can_capabilities{};
        PFIFOWRITER _can_writer = nullptr;
    };
} // namespace Uni::CAN

#endif
