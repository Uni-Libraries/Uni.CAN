#if defined(_WIN32)

// stdlib
#include <chrono>
#include <cstring>
#include <thread>

// windows
#include <windows.h>

// PCanBasic
#include <vxlapi.h>

// Uni.CAN
#include "backend_vector/can_vector_channel.h"

using namespace std::chrono_literals;

namespace Uni::CAN {
    CanChannelVector::CanChannelVector(uni_can_devinfo_t* devInfo, uint32_t baudrate) {
        _info_dev = *devInfo;
        _info_chidx = 0U;
        _info_baudrate = baudrate;
    }

    CanChannelVector::~CanChannelVector() {
        Close();
        DeInit();
    }



    //
    // Thread
    //

    void CanChannelVector::threadProc()
    {
        XLhandle event;
        xlSetNotification (_device_handle, &event, 1);

        while (!_thread_abort) {
            threadProcReceive(event);
        }
    }

    void CanChannelVector::threadProcReceive(void* event) {
        if (WaitForSingleObject(event, _threadWaitTime) == WAIT_OBJECT_0) {
            XLstatus        xlStatus = XL_SUCCESS;
            do {
                XLcanRxEvent    xlCanRxEvt;
                xlStatus = xlCanReceive(_device_handle, &xlCanRxEvt);

                if(xlStatus==XL_ERR_QUEUE_IS_EMPTY ) {
                    break;
                }

                if (xlCanRxEvt.tag != XL_RECEIVE_MSG) {
                    continue;
                }

                if (xlCanRxEvt.tagData.canRxOkMsg.msgFlags & XL_CAN_MSG_FLAG_ERROR_FRAME)
                {
                    continue;
                }

                auto* msg = uni_can_message_create();
                if (xlCanRxEvt.tagData.canRxOkMsg.canId & XL_CAN_EXT_MSG_ID) {
                    msg->flags = UNI_CAN_MSG_FLAG_EXT_ID;
                }
                else {
                    msg->flags = UNI_CAN_MSG_FLAG_STD_ID;
                }

                msg->id = xlCanRxEvt.tagData.canRxOkMsg.canId & (XL_CAN_EXT_MSG_ID-1);
                msg->len = xlCanRxEvt.tagData.canRxOkMsg.dlc;
                memcpy(msg->data.u8, xlCanRxEvt.tagData.canRxOkMsg.data, msg->len);

                msg->time_us = xlCanRxEvt.timeStampSync / 1'000;

                m_receive_queue.push(msg);
                if (m_receive_func) {
                    m_receive_func(this, m_receive_cookie);
                }

            } while(XL_SUCCESS == xlStatus);
        }
    }

    //
    // Transmit
    //

    bool CanChannelVector::TransmitMessage(const uni_can_message_t &msg) {
        unsigned int msg_count = 1;
        XLevent event{};
        event.tag = XL_TRANSMIT_MSG;
        event.tagData.msg.id = msg.id;
        if (msg.flags & UNI_CAN_MSG_FLAG_EXT_ID) {
            event.tagData.msg.id |= XL_CAN_EXT_MSG_ID;
        }
        event.tagData.msg.dlc = msg.len;
        memcpy(event.tagData.msg.data, msg.data.u8, msg.len);

        return xlCanTransmit(_device_handle, _info_dev.device_index, &msg_count, &event) == XL_SUCCESS;
    }


    bool CanChannelVector::DeInit() {
        return true;
    }

    bool CanChannelVector::Init() {
        XLstatus status{};

        //Open port
        XLportHandle handle;
        XLaccess permission = _info_dev.device_index;

        char name[] = "Uni.CAN";
        status = xlOpenPort(&handle, name, _info_dev.device_index, &permission, 4096, XL_INTERFACE_VERSION, XL_BUS_TYPE_CAN);
        if(status != XL_SUCCESS)
        {
            return false;
        }

        // Check port
        status = xlCanRequestChipState(handle, _info_dev.device_index);
        if (status != XL_SUCCESS)
        {
            return false;
        }

        //Set Channel mode
        status = xlCanSetChannelMode(handle, _info_dev.device_index, 0, 0);
        if (status != XL_SUCCESS)
        {
            return false;
        }

        // Set Bitrate
        status = xlCanSetChannelBitrate(handle, _info_dev.device_index, _info_baudrate);
        if (status != XL_SUCCESS)
        {
            return false;
        }

        _device_handle = handle;
        return true;
    }

    bool CanChannelVector::Open() {
        m_receive_queue.clear();

        // Activate channel
        if (xlActivateChannel(_device_handle, _info_dev.device_index, XL_BUS_TYPE_CAN, XL_ACTIVATE_NONE) != XL_SUCCESS) {
            return false;
        }

        // Reset Clock
        if (xlResetClock(_device_handle) != XL_SUCCESS) {
            return false;
        }

        // start thread
        threadStart();

        return true;
    }

    bool CanChannelVector::Close() {
        threadStop(); 
        m_receive_queue.clear();
        return xlDeactivateChannel(_device_handle, _info_dev.device_index) == XL_SUCCESS;
    }

} // namespace Uni::CAN

#endif
