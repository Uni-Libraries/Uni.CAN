//
// Includes
//

// uni.can
#include "can_channel_base.h"



//
// Implementation
//


namespace Uni::CAN
{
    //
    // Receive
    //
    uni_can_message_t * CanChannelBase::ReceiveMessage() {
        if (m_receive_queue.empty()) {
            return nullptr;
        }

        return m_receive_queue.pop();
    }

    void CanChannelBase::ReceiveHandlerSet(uni_can_channel_receive_handler_f func, void *cookie) {
        m_receive_func = func;
        m_receive_cookie = cookie;
    }


    bool CanChannelBase::threadStop()
    {
        if (!_thread.joinable()) {
            return false;
        }
        _thread.join();
        return true;
    }

    bool CanChannelBase::threadStart() {
        if (_thread.joinable()) {
            return false;
        }
        _thread = std::thread(&CanChannelBase::threadProc, this);
        return true;
    }

}
