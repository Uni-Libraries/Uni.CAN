#pragma once

//
// Includes
//

// stdlib
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <functional>

// C++
#include <variant>

// Uni.CAN
#include <uni_can.h>

// app
#include "can_types.h"
#include "can_mgr_state.h"


//
// Typedefs
//



namespace APP {
    class CanManager {
    public:
        virtual ~CanManager();

    public:
        struct ConnectParams {
            size_t device_index{};
            size_t baudrate{};
            ParseBackend backend{ParseBackend::RawCan};
            ProtoPlexerConfig protoplexer{};
        };

        // Backward-compatible default: raw CAN.
        bool Connect(size_t idx, size_t baudrate);
        bool Connect(const ConnectParams& params);
        void Disconnect();
        [[nodiscard]] bool IsConnected() const;

        // UI-configured (can be changed in STANDBY too). Applied on next Connect().
        [[nodiscard]] ParseBackend BackendConfiguredGet() const;
        [[nodiscard]] ParseBackend& BackendConfiguredGet();
        [[nodiscard]] const ProtoPlexerConfig& ProtoPlexerConfigConfiguredGet() const;
        [[nodiscard]] ProtoPlexerConfig& ProtoPlexerConfigConfiguredGet();

        // Active backend of current connection.
        [[nodiscard]] ParseBackend BackendGet() const;
        [[nodiscard]] const ProtoPlexerConfig& ProtoPlexerConfigGet() const;

        // TX
        void SendMessage(const uni_can_message_t* msg);
        void SendProtoPlexer(const ProtoPlexerMessage& msg);
    protected:
        virtual void transmitAction(uni_can_message_t &msg);
    private:
        void *m_can_channel{};

        // Parsing backend
    private:
        ParseBackend m_backend_cfg{ParseBackend::RawCan};
        ProtoPlexerConfig m_pp_cfg_cfg{};

        ParseBackend m_backend_active{ParseBackend::RawCan};
        ProtoPlexerConfig m_pp_cfg_active{};

        uni_can_protoplexer_ctx_t m_pp_ctx{};
        bool m_pp_inited{false};

        std::queue<TxPacket> m_tx_queue;
        std::mutex m_tx_queue_mtx;


        // Info
    public:
        void InfoRefresh();
        size_t InfoGetCount();
        std::string InfoGetName(size_t idx);
    private:
        std::vector<std::string> m_info_names;

        // Receive
    public:
        using RxHandler = std::function<void(const RxPacket&)>;

        void ReceiveSubscribe(std::string_view id, RxHandler function);
        void ReceiveUnsubscribe(std::string_view id);
    private:
        void receiveAction(const RxPacket& msg);
    private:
        std::map<std::string, RxHandler> m_receive_subs;
        std::mutex m_receive_subs_mutex;


        // State
    public:
        [[nodiscard]] CanManagerState StateGet() const;
    private:
        void stateSet(CanManagerState val);
    private:
        CanManagerState m_state{CanManagerState::STANDBY};


        // Thread
    public:
        void ThreadNotify();
    private:
        void threadFunction();
        void threadFunctionOperating();
    private:
        std::thread m_thread;
        std::condition_variable m_thread_cv;
        std::mutex m_thread_mtx;

        bool m_thread_abort{};

        // Transmit
    public:
        uint32_t& TransmitThrottle();
    private:
        uint32_t m_transmit_throttle{};
    };
}
