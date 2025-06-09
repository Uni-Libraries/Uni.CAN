#if defined(_WIN32)

// stdlib
#include <array>
#include <chrono>
#include <cstring>
#include <thread>

// windows
#include <windows.h>

// waveshare
#include <ControlCAN.h>

// Uni.CAN
#include "backend_Waveshare/can_waveshare_channel.h"

using namespace std::chrono_literals;

namespace Uni::CAN {
    CanChannelWaveshare::CanChannelWaveshare(uni_can_devinfo_t* devInfo, uint32_t chan_idx, uint32_t baudrate) {
        _info_dev = *devInfo;
        _info_chidx = chan_idx;
        _info_baudrate = baudrate;
    }

    CanChannelWaveshare::~CanChannelWaveshare() {
        Close();
        DeInit();
    }



    //
    // Thread
    //

    void CanChannelWaveshare::threadProc()
    {
        while (!_thread_abort) {
            threadProcReceive();
        }
    }

    void CanChannelWaveshare::threadProcReceive() {
        std::array<VCI_CAN_OBJ, 128> msg_rcv;
        int rcv_len = 0;
        do
        {
            if((rcv_len=VCI_Receive(VCI_USBCAN2,_info_dev.device_index, _info_chidx, msg_rcv.data(), msg_rcv.size(),_threadWaitTime))>0)
            {
                for(size_t idx = 0; idx< rcv_len; idx++)
                {
                    auto* msg = uni_can_message_create();
                    if (msg_rcv[idx].ExternFlag) {
                        msg->flags = UNI_CAN_MSG_FLAG_EXT_ID;
                    }
                    else {
                        msg->flags = UNI_CAN_MSG_FLAG_STD_ID;
                    }

                    msg->id = msg_rcv[idx].ID;
                    msg->len = msg_rcv[idx].DataLen;
                    memcpy(msg->data.u8, msg_rcv[idx].Data, msg->len);

                    if(msg_rcv[idx].TimeFlag){
                        msg->time_us = msg_rcv[idx].TimeStamp * 100U;
                    }

                    m_receive_queue.push(msg);
                    if (m_receive_func) {
                        m_receive_func(this, m_receive_cookie);
                    }
                }
            }
        } while (rcv_len > 0);
    }

    //
    // Transmit
    //

    bool CanChannelWaveshare::TransmitMessage(const uni_can_message_t &msg) {
        VCI_CAN_OBJ send{};
        send.ID=msg.id;
        send.SendType=0;
        send.RemoteFlag=0;
        send.ExternFlag= ((msg.flags & UNI_CAN_MSG_FLAG_EXT_ID) == UNI_CAN_MSG_FLAG_EXT_ID) ? 1 : 0;
        send.DataLen=msg.len;
        memcpy(send.Data, msg.data.u8, send.DataLen);
        return VCI_Transmit(VCI_USBCAN2, _info_dev.device_index, _info_chidx, &send, 1U) == 1;
    }


    bool CanChannelWaveshare::DeInit() {

        return true;
    }

    bool CanChannelWaveshare::Init() {
       return true;
    }

    bool CanChannelWaveshare::Open() {
        m_receive_queue.clear();

        VCI_INIT_CONFIG init_config{};
        init_config.AccCode = 0;
        init_config.AccMask = 0xFFFFFFFF;
        init_config.Filter = 1;
        init_config.Mode = 0;
        switch (_info_baudrate)
        {
            case 100'000:
                init_config.Timing0 = 0x04;
                init_config.Timing1 = 0x1C;
                break;
            case 125'000:
                init_config.Timing0 = 0x03;
                init_config.Timing1 = 0x1C;
                break;
            case 250'000:
                init_config.Timing0 = 0x01;
                init_config.Timing1 = 0x1C;
                break;
            case 500'000:
                init_config.Timing0 = 0x00;
                init_config.Timing1 = 0x1C;
                break;
            case 800'000:
                init_config.Timing0 = 0x00;
                init_config.Timing1 = 0x16;
                break;
            case 1'000'000:
                init_config.Timing0 = 0x00;
                init_config.Timing1 = 0x14;
                break;
            default:
                return false;
        }

        if(VCI_OpenDevice(VCI_USBCAN2, _info_dev.device_index, 0U) != 1){
            return false;
        }

        if (VCI_ResetCAN(VCI_USBCAN2, _info_dev.device_index, _info_chidx) != 1) {
            return false;
        }

        if(VCI_InitCAN(VCI_USBCAN2, _info_dev.device_index, _info_chidx, &init_config) != 1){
            return false;
        }

        if(VCI_StartCAN(VCI_USBCAN2, _info_dev.device_index, _info_chidx) != 1){
            return false;
        }

        // start thread
        threadStart();

        return true;
    }

    bool CanChannelWaveshare::Close() {
        threadStop(); 
        m_receive_queue.clear();
        if(VCI_CloseDevice(VCI_USBCAN2, _info_dev.device_index) != 1){
            return false;
        }

        return true;
    }

} // namespace Uni::CAN

#endif
