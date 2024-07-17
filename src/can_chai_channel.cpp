#if defined(_WIN32)

// stdlib
#include <cstring>

// CHAI SDK
#include <chai.h>

// auris.can --> private
#include "can_chai_channel.h"

namespace Auris::CAN {
    CanChannelChai::~CanChannelChai() {
        Close();
        DeInit();
    }

    bool CanChannelChai::ReceiveMessage(CanMessage &msg) {
        canmsg_t canmsg_native{};
        bool result = false;

        if (CiRead(_channel_num, &canmsg_native, 1) == 1) {
            msg.msg_id = canmsg_native.id;
            msg.msg_len = canmsg_native.len;
            memcpy(msg.msg_data, canmsg_native.data, msg.msg_len);
            result = true;
        }

        return result;
    }

    bool CanChannelChai::TransmitMessage(CanMessage &msg) {
        canmsg_t output_frame{};
        output_frame.id = msg.msg_id;
        output_frame.len = msg.msg_len;
        output_frame.flags = FRAME_EFF;
        output_frame.ts = 0;

        memcpy(output_frame.data, msg.msg_data, msg.msg_len);
        return CiTransmit(_channel_num, &output_frame) == 0;
    }

    CanChannelChai::CanChannelChai(std::shared_ptr<CanDevinfoChai> &devInfo, size_t channelIdx, uint32_t baudrate) {
        _dev_info = devInfo;
        _channel_num = _dev_info->GetChannelNum(channelIdx);
        _can_baudrate = baudrate;
    }

    bool CanChannelChai::DeInit() { return CiClose(_channel_num) == 0; }

    bool CanChannelChai::Init() {
        if (CiOpen(_channel_num, CIO_CAN11 | CIO_CAN29) != 0) {
            return false;
        }

        uint8_t bt0 = 0;
        uint8_t bt1 = 0;

        switch (_can_baudrate) {
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

        if (CiSetBaud(_channel_num, bt0, bt1) != 0) {
            return false;
        }

        return true;
    }

    bool CanChannelChai::Open() {
        if (CiStart(_channel_num) != 0) {
            return false;
        }

        return true;
    }

    bool CanChannelChai::Close() { return CiStop(_channel_num) == 0; }
} // namespace Auris::CAN

#endif
