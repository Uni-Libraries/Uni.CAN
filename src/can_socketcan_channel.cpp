#if defined(__linux__)

// stdlib
#include <array>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

// linux
#include <linux/can.h>
#include <linux/can/error.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/poll.h>
#include <sys/eventfd.h>

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


    bool CanChannelSocketcan::TransmitMessage(const uni_can_message_t &msg) {
        if (_fd < 0) {
            return false;
        }

        can_frame frame{};
        if(msg.flags & UNI_CAN_MSG_FLAG_EXT_ID) {
            frame.can_id = (msg.id & CAN_EFF_MASK) | CAN_EFF_FLAG;
        }
        else if(msg.flags & UNI_CAN_MSG_FLAG_STD_ID) {
            frame.can_id = (msg.id & CAN_SFF_MASK);
        }

        frame.can_dlc = msg.len;
        memcpy(frame.data, msg.data.u8, sizeof(frame.data));

        if (write(_fd, &frame, sizeof(frame)) < 0) {
            return false;
        }

        return true;
    }


    bool CanChannelSocketcan::DeInit() { return true; }

    bool CanChannelSocketcan::Init() { return true; }

    CanChannelSocketcan::CanChannelSocketcan(const uni_can_devinfo_t* devInfo, int channel_idx, uint32_t baudrate) {
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
        if(_fd < 0) {
            return false;
        }

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

        // set nowait
        fcntl(_fd, F_SETFL, O_NONBLOCK);

        // start thread
        threadStart();

        return true;
    }

    bool CanChannelSocketcan::Close() {
        threadStop();
        shutdown(_fd, SHUT_RDWR);
        close(_fd);
        _fd = -1;
        return true;
    }



    //
    // Receive
    //

    uni_can_message_t * CanChannelSocketcan::ReceiveMessage() {
        if (m_receive_queue.empty()) {
            return nullptr;
        }

        return m_receive_queue.pop();
    }

    void CanChannelSocketcan::ReceiveHandlerSet(uni_can_channel_receive_handler_f func, void *cookie) {
        m_receive_func = func;
        m_receive_cookie = cookie;
    }


    bool CanChannelSocketcan::receiveMessage() {
        // Read frame
        can_frame frame{};
        ssize_t bytesRead = read(_fd, &frame, sizeof(can_frame));
        if (bytesRead != sizeof(frame)) {
            return false;
        }

        // Check error
        if ((frame.can_id & CAN_ERR_FLAG) != 0) {
            return true;
        }

        // create msg
        auto* msg = uni_can_message_create();
        if(!msg) {
            return true;
        }

        // Strip flags
        if ((frame.can_id & CAN_EFF_FLAG) != 0) {
            frame.can_id &= CAN_EFF_MASK;
            msg->flags = static_cast<uni_can_message_flags_t>(msg->flags | UNI_CAN_MSG_FLAG_EXT_ID);
        } else {
            frame.can_id &= CAN_SFF_MASK;
        }

        // Fill struct
        msg->id = frame.can_id;
        msg->len = frame.can_dlc;
        memcpy(msg->data.u8, frame.data, sizeof(msg->data));

        // populate
        m_receive_queue.push(msg);

        if(m_receive_func) {
            m_receive_func(this, m_receive_cookie);
        }

        return true;
    }



    //
    // Thread
    //

    void CanChannelSocketcan::threadProc() {
        _thread_fd = eventfd(0, EFD_NONBLOCK);
        if(_thread_fd < 0){
            return;
        }

        std::array<pollfd, 2> poll_fds{};
        poll_fds[0].fd = _fd;
        poll_fds[0].events = POLLIN;
        poll_fds[1].fd = _thread_fd;
        poll_fds[1].events = POLLIN;

        while (true) {
            if(poll(poll_fds.data(), poll_fds.size(), -1)>0) {
                if (poll_fds[0].revents & POLLIN) {
                    while(receiveMessage()) {
                        /* noop */
                    }
                }
                if (poll_fds[1].revents & POLLIN) {
                    break;
                }
            }
        }

        close(_thread_fd);
        _thread_fd = -1;
    }

    bool CanChannelSocketcan::threadStop() {
        if (!_thread.joinable()) {
            return false;
        }
        if(_thread_fd>-1) {
            eventfd_write(_thread_fd, 1);
        }
        _thread.join();
        return true;
    }

    bool CanChannelSocketcan::threadStart() {
        if (_thread.joinable()) {
            return false;
        }
        _thread = std::thread(&CanChannelSocketcan::threadProc, this);
        return true;
    }

} // namespace Uni::CAN

#endif
