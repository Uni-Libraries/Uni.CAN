#if defined(_WIN32)

// stdlib
#include <chrono>
#include <cstring>
#include <thread>

// windows
#include <windows.h>

// PCanBasic
#include <PCANBasic.h>

// Uni.CAN
#include "backend_peak/can_peak_channel.h"

using namespace std::chrono_literals;

namespace Uni::CAN {
    CanChannelPeak::CanChannelPeak(uni_can_devinfo_t* devInfo, uint32_t baudrate) {
        _info_dev = *devInfo;
        _info_chidx = 0U;
        _info_baudrate = baudrate;
    }

    CanChannelPeak::~CanChannelPeak() {
        Close();
        DeInit();
    }



    //
    // Thread
    //

    void CanChannelPeak::threadProc()
    {
        HANDLE event = CreateEventW(nullptr, FALSE, FALSE, L"");
        CAN_SetValue(_info_dev.device_index, PCAN_RECEIVE_EVENT, &event, sizeof(event));

        while (!_thread_abort) {
            threadProcReceive(event);
        }

        HANDLE event_clr = nullptr;
        CAN_SetValue(_info_dev.device_index, PCAN_RECEIVE_EVENT, &event_clr, sizeof(event_clr));

        CloseHandle(event);
    }

    void CanChannelPeak::threadProcReceive(void* event) {
        if (WaitForSingleObject(event, _threadWaitTime) == WAIT_OBJECT_0) {
            TPCANStatus stsResult;
            do
            {
                TPCANMsg msg_p;
                TPCANTimestamp time_p;
                stsResult = CAN_Read(_info_dev.device_index, &msg_p, &time_p);
                if (stsResult != PCAN_ERROR_QRCVEMPTY) {
                    if ((msg_p.MSGTYPE & PCAN_MESSAGE_STATUS) == PCAN_MESSAGE_STATUS) {
                        continue;
                    }
                    if ((msg_p.MSGTYPE & PCAN_MESSAGE_ERRFRAME) == PCAN_MESSAGE_ERRFRAME) {
                        continue;
                    }

                    auto* msg = uni_can_message_create();
                    if ((msg_p.MSGTYPE & PCAN_MESSAGE_EXTENDED) == PCAN_MESSAGE_EXTENDED) {
                        msg->flags = UNI_CAN_MSG_FLAG_EXT_ID;
                    }
                    else {
                        msg->flags = UNI_CAN_MSG_FLAG_STD_ID;
                    }

                    msg->id = msg_p.ID;
                    msg->len = msg_p.LEN;
                    memcpy(msg->data.u8, msg_p.DATA, msg->len);

                    msg->time_us = time_p.micros + (1000ULL * time_p.millis) + (0x100000000ULL * 1000ULL * time_p.millis_overflow);

                    m_receive_queue.push(msg);
                    if (m_receive_func) {
                        m_receive_func(this, m_receive_cookie);
                    }
                }
            } while (!(stsResult & PCAN_ERROR_QRCVEMPTY));
        }
    }

    //
    // Transmit
    //

    bool CanChannelPeak::TransmitMessage(const uni_can_message_t &msg) {
        TPCANMsg msg_p;
        msg_p.ID = msg.id;
        msg_p.LEN = msg.len;
        memcpy(msg_p.DATA, msg.data.u8, msg_p.LEN);
        msg_p.MSGTYPE = (msg.flags & UNI_CAN_MSG_FLAG_EXT_ID) ? PCAN_MESSAGE_EXTENDED : 0U;
        return CAN_Write(_info_dev.device_index, &msg_p) == PCAN_ERROR_OK;
    }


    bool CanChannelPeak::DeInit() {
        return true;
    }

    bool CanChannelPeak::Init() {
       return true;
    }

    bool CanChannelPeak::Open() {
        m_receive_queue.clear();

        auto bitrate = PCAN_BAUD_1M;
        switch (_info_baudrate)
        {
            case 100'000:
                bitrate = PCAN_BAUD_100K;
                break;
            case 125'000:
                bitrate = PCAN_BAUD_125K;
                break;
            case 250'000:
                bitrate = PCAN_BAUD_250K;
                break;
            case 500'000:
                bitrate = PCAN_BAUD_500K;
                break;
            case 800'000:
                bitrate = PCAN_BAUD_800K;
                break;
            case 1'000'000:
                bitrate = PCAN_BAUD_1M;
                break;
            default:
                return false;
        }

        if (CAN_Initialize(_info_dev.device_index, bitrate) != PCAN_ERROR_OK) {
            return false;
        }

        // start thread
        threadStart();

        return true;
    }

    bool CanChannelPeak::Close() {
        threadStop(); 
        m_receive_queue.clear();
        return CAN_Uninitialize(_info_dev.device_index) == PCAN_ERROR_OK;
    }

} // namespace Uni::CAN

#endif
