#pragma once

#if defined(_MSC_VER)

// stdlib
#include <deque>
#include <memory>
#include <thread>

// IXXAT VCI SDK
#include <vcisdk.h>

// auris.can --> public
#include "auris/can/can_channel.h"

// auris.can --> private
#include "can_ixxat_devinfo.h"
#include "common_queue.h"

namespace Auris::CAN {
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

        bool ReceiveMessage(CanMessage &msg) override;
        bool TransmitMessage(CanMessage &msg) override;

        //
        // Ctor
        //
      protected:
        friend class CanProviderIxxat;
        explicit CanChannelIxxat(std::shared_ptr<CanDevinfoIxxat> &devInfo, size_t channelIdx, uint32_t baudrate);

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
        SharedQueue<CanMessage> _read_queue;

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
        std::shared_ptr<CanDevinfoIxxat> _dev_info;

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
} // namespace Auris::CAN

#endif
