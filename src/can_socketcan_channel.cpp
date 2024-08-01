#if defined(__linux__)

// stdlib
#include <cstring>
#include <unistd.h>

// linux
#include <linux/can.h>
#include <linux/can/error.h>
#include <linux/can/raw.h>
#include <net/if.h>

// libsocketcan
#include <libsocketcan.h>

// Uni.CAN
#include "can_socketcan_channel.h"



namespace Uni::CAN {
    //
    // Public
    //

    CanChannelSocketcan::~CanChannelSocketcan() {
        Close();
        DeInit();
    }

    bool CanChannelSocketcan::ReceiveMessage(uni_can_message_t &msg) {
        if (_fd < 0) {
            return false;
        }

        // Set up a file descriptor set only containing one socket
        fd_set fds_read{};
        FD_ZERO(&fds_read);
        FD_SET(_fd, &fds_read);

        // Use select to be able to use a timeout
        timeval timeout{};
        if (select(_fd + 1, &fds_read, nullptr, nullptr, &timeout) <= 0) {
            return false;
        }

        // Read frame
        can_frame frame{};
        ssize_t bytesRead = read(_fd, &frame, sizeof(can_frame));
        if (bytesRead != sizeof(frame)) {
            return false;
        }

        // Check error
        if ((frame.can_id & CAN_ERR_FLAG) != 0) {
            return false;
        }

        // Strip flags
        if ((frame.can_id & CAN_EFF_FLAG) != 0) {
            frame.can_id &= CAN_EFF_MASK;
        } else {
            frame.can_id &= CAN_SFF_MASK;
        }

        // Fill struct
        msg.id = frame.can_id;
        msg.len = frame.can_dlc;
        memcpy(msg.data.u8, frame.data, sizeof(msg.data));

        return true;
    }

    bool CanChannelSocketcan::TransmitMessage(const uni_can_message_t &msg) {
        if (_fd < 0) {
            return false;
        }

        can_frame frame{};
        frame.can_id = msg.id | CAN_EFF_FLAG;
        frame.can_dlc = msg.len;
        memcpy(frame.data, msg.data.u8, sizeof(frame.data));

        if (write(_fd, &frame, sizeof(frame)) < 0) {
            return false;
        }

        return true;
    }

    bool CanChannelSocketcan::DeInit() { return true; }

    bool CanChannelSocketcan::Init() { return true; }

    CanChannelSocketcan::CanChannelSocketcan(uni_can_devinfo_t* devInfo, int channel_idx, uint32_t baudrate) {
        memcpy(&_dev_info, devInfo, sizeof(uni_can_devinfo_t));
        _can_baudrate = baudrate;
    }

    bool CanChannelSocketcan::Open() {
        const auto *can_name = _dev_info.device_sn;

        // stop interface
        can_do_stop(can_name);

        // set bittiming
        can_bittiming bittiming{};
        can_get_bittiming(can_name, &bittiming);
        if (bittiming.bitrate != _can_baudrate) {
            bittiming.bitrate = _can_baudrate;
            if (can_set_bittiming(can_name, &bittiming) < 0) {
                return false;
            }
        }

        // set restart-ms
        uint32_t restart_ms;
        if(can_get_restart_ms(can_name, &restart_ms) == 0){
            if(restart_ms != _can_restart) {
                if(can_set_restart_ms(can_name, _can_restart) < 0) {
                    return false;
                }
            }
        }

        // start interface
        can_do_start(can_name);

        // open socket
        if (_fd >= 0) {
            return false;
        }
        _fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);

        // bind socket
        sockaddr_can addr{};
        addr.can_family = AF_CAN;
        addr.can_ifindex = _dev_info.device_index;
        if (bind(_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == -1) {
            _fd = -1;
            return false;
        }

        // disable CAN loopback
        int loopback = 0; /* 0 = disabled, 1 = enabled (default) */
        setsockopt(_fd, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback));

        return true;
    }

    bool CanChannelSocketcan::Close() {
        shutdown(_fd, SHUT_RDWR);
        close(_fd);
        _fd = -1;
        return true;
    }
} // namespace Uni::CAN

#endif
