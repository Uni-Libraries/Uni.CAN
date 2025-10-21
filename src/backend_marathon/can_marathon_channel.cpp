#if defined(_WIN32)

// stdlib
#include <chrono>
#include <cstring>
#include <thread>

// CHAI SDK
#include <chai.h>

// Uni.CAN
#include "can_marathon_channel.h"

using namespace std::chrono_literals;

namespace Uni::CAN {
    CanChannelMarathon::CanChannelMarathon(uni_can_devinfo_t* devInfo, size_t channelIdx, uint32_t baudrate) {
        _info_dev = *devInfo;
        _info_chidx = channelIdx;
        _info_baudrate = baudrate;
    }

    CanChannelMarathon::~CanChannelMarathon() {
        Close();
        DeInit();
    }



    //
    // Thread
    //

    void CanChannelMarathon::threadProc()
    {
        while (!_thread_abort) {
            threadProcReceive();
        }
    }

    bool CanChannelMarathon::threadProcReceive() {
        canwait_t cw;
        cw.chan = _info_chidx;
        cw.wflags = CI_WAIT_RC;
        if (CiWaitEvent(&cw, 1, 10) > 0) {

            canmsg_t canmsg_native{};
            if (CiRead(_info_chidx, &canmsg_native, 1) != 1) {
                return false;
            }

            auto* msg = uni_can_message_create();
            msg->id = canmsg_native.id;
            msg->len = canmsg_native.len;
            msg->time_us = canmsg_native.ts;
            memcpy(msg->data.u8, canmsg_native.data, msg->len);

            m_receive_queue.push(msg);
            if (m_receive_func) {
                m_receive_func(this, m_receive_cookie);
            }
        }

        return true;
    }

    //
    // Transmit
    //

    bool CanChannelMarathon::TransmitMessage(const uni_can_message_t &msg) {
        canmsg_t output_frame{};
        output_frame.id = msg.id;
        output_frame.len = msg.len;
        output_frame.flags = FRAME_EFF;
        output_frame.ts = 0;

        memcpy(output_frame.data, msg.data.u8, msg.len);
        return CiTransmit(_info_chidx, &output_frame) == 0;
    }


    bool CanChannelMarathon::DeInit() { return CiClose(_info_chidx) == 0; }

    bool CanChannelMarathon::Init() {
        if (CiOpen(_info_chidx, CIO_CAN11 | CIO_CAN29) != 0) {
            return false;
        }

        uint8_t bt0 = 0;
        uint8_t bt1 = 0;

        switch (_info_baudrate) {
        case 10'000:
            bt0 = BCI_10K_bt0;
            bt1 = BCI_10K_bt1;
            break;
        case 20'000:
            bt0 = BCI_20K_bt0;
            bt1 = BCI_20K_bt1;
            break;
        case 50'000:
            bt0 = BCI_50K_bt0;
            bt1 = BCI_50K_bt1;
            break;
        case 100'000:
            bt0 = BCI_100K_bt0;
            bt1 = BCI_100K_bt1;
            break;
        case 125'000:
            bt0 = BCI_125K_bt0;
            bt1 = BCI_125K_bt1;
            break;
        case 250'000:
            bt0 = BCI_250K_bt0;
            bt1 = BCI_250K_bt1;
            break;
        case 500'000:
            bt0 = BCI_500K_bt0;
            bt1 = BCI_500K_bt1;
            break;
        case 800'000:
            bt0 = BCI_800K_bt0;
            bt1 = BCI_800K_bt1;
            break;
        case 1'000'000:
            bt0 = BCI_1M_bt0;
            bt1 = BCI_1M_bt1;
            break;
        default:
            break;
        }

        if (CiSetBaud(_info_chidx, bt0, bt1) != 0) {
            return false;
        }

        return true;
    }

    bool CanChannelMarathon::Open() {
        m_receive_queue.clear();

        if (CiStart(_info_chidx) != 0) {
            return false;
        }

        // start thread
        threadStart();

        return true;
    }

    bool CanChannelMarathon::Close() {
        threadStop(); 
        m_receive_queue.clear();
        return CiStop(_info_chidx) == 0;
    }

} // namespace Uni::CAN

#endif
