//
// Includes
//

// stdlib
#include <utility>

// uni.can
#include <uni_can.h>

// app
#include "can_mgr.h"


namespace APP
{
    static void receive_handler(void* /*channel*/, void* cookie)
    {
        static_cast<CanManager*>(cookie)->ThreadNotify();
    }

    CanManager::~CanManager()
    {
        Disconnect();
    }

    bool CanManager::Connect(size_t idx, size_t baudrate)
    {
        ConnectParams params{};
        params.device_index = idx;
        params.baudrate = baudrate;
        params.backend = ParseBackend::RawCan;
        return Connect(params);
    }

    bool CanManager::Connect(const ConnectParams& params)
    {
        Disconnect();

        // Cache UI-selected configuration for later (even if connect fails).
        m_backend_cfg = params.backend;
        m_pp_cfg_cfg = params.protoplexer;

        if (m_info_names.empty())
        {
            InfoRefresh();
        }

        if (params.device_index >= m_info_names.size())
        {
            stateSet(CanManagerState::CAN_ERROR);
            return false;
        }

        stateSet(CanManagerState::CONNECTING);

        bool result{ false };
        uni_can_devinfo_t info{};
        if (uni_can_factory_get_info(&info, params.device_index))
        {
            m_can_channel = uni_can_factory_create_channel(&info, 0, params.baudrate);
            if (m_can_channel)
            {
                if (uni_can_channel_init(m_can_channel))
                {
                    uni_can_channel_set_receive_handler(m_can_channel, &receive_handler, this);
                    result = uni_can_channel_open(m_can_channel);
                }
            }
        }

        if (!result)
        {
            Disconnect();
            stateSet(CanManagerState::CAN_ERROR);
            return false;
        }

        // backend init (active)
        m_backend_active = params.backend;
        m_pp_cfg_active = params.protoplexer;

        if (m_backend_active == ParseBackend::ProtoPlexer)
        {
            uni_can_protoplexer_config_t cfg{};
            cfg.own_address = m_pp_cfg_active.own_address;
            cfg.max_chunk_length = m_pp_cfg_active.max_chunk_length;
            cfg.max_channels = m_pp_cfg_active.max_channels;
            cfg.monitoring = m_pp_cfg_active.monitoring;

            m_pp_inited = uni_can_protoplexer_init(&m_pp_ctx, &cfg);
            if (!m_pp_inited)
            {
                Disconnect();
                stateSet(CanManagerState::CAN_ERROR);
                return false;
            }
        }

        stateSet(CanManagerState::OPERATING);

        // start thread
        m_thread = std::thread(&CanManager::threadFunction, this);
        return true;
    }

    void CanManager::Disconnect()
    {
        m_thread_abort = true;
        ThreadNotify();
        if (m_thread.joinable())
        {
            m_thread.join();
        }

        if (m_can_channel)
        {
            uni_can_channel_close(m_can_channel);
            uni_can_channel_destroy(m_can_channel);
            m_can_channel = nullptr;
        }

        if (m_pp_inited)
        {
            uni_can_protoplexer_deinit(&m_pp_ctx);
            m_pp_inited = false;
        }

        {
            std::lock_guard<std::mutex> lock(m_tx_queue_mtx);
            while (!m_tx_queue.empty())
            {
                m_tx_queue.pop();
            }
        }

        m_backend_active = ParseBackend::RawCan;
        m_pp_cfg_active = {};

        stateSet(CanManagerState::STANDBY);
    }

    bool CanManager::IsConnected() const
    {
        return m_can_channel != nullptr;
    }

    ParseBackend CanManager::BackendGet() const
    {
        return m_backend_active;
    }

    const ProtoPlexerConfig& CanManager::ProtoPlexerConfigGet() const
    {
        return m_pp_cfg_active;
    }

    ParseBackend CanManager::BackendConfiguredGet() const
    {
        return m_backend_cfg;
    }

    ParseBackend& CanManager::BackendConfiguredGet()
    {
        return m_backend_cfg;
    }

    const ProtoPlexerConfig& CanManager::ProtoPlexerConfigConfiguredGet() const
    {
        return m_pp_cfg_cfg;
    }

    ProtoPlexerConfig& CanManager::ProtoPlexerConfigConfiguredGet()
    {
        return m_pp_cfg_cfg;
    }

    void CanManager::SendMessage(const uni_can_message_t* msg)
    {
        if (!msg)
        {
            return;
        }

        // Keep TP safe: clone allocates msg and optional data.
        CanMessagePtr ptr(uni_can_message_clone(msg), &uni_can_message_free);
        if (!ptr)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(m_tx_queue_mtx);
        m_tx_queue.emplace(std::move(ptr));
        ThreadNotify();
    }

    void CanManager::SendProtoPlexer(const ProtoPlexerMessage& msg)
    {
        std::lock_guard<std::mutex> lock(m_tx_queue_mtx);
        m_tx_queue.emplace(msg);
        ThreadNotify();
    }

    //
    // Info
    //

    size_t CanManager::InfoGetCount()
    {
        if (m_info_names.empty())
        {
            InfoRefresh();
        }
        return m_info_names.size();
    }

    void CanManager::InfoRefresh()
    {
        auto size = uni_can_factory_refresh();
        m_info_names.clear();

        for (size_t idx = 0; idx < size; idx++)
        {
            uni_can_devinfo_t info{};
            uni_can_factory_get_info(&info, idx);
            m_info_names.push_back(
                std::string(info.device_manufacturer) + " " + info.device_model + " " + info.device_sn);
        }
    }

    std::string CanManager::InfoGetName(size_t idx)
    {
        if (idx >= InfoGetCount())
        {
            return "";
        }
        return m_info_names[idx];
    }

    //
    // Receive
    //

    void CanManager::ReceiveSubscribe(std::string_view id, RxHandler function)
    {
        std::lock_guard lock(m_receive_subs_mutex);
        m_receive_subs.emplace(id, std::move(function));
    }

    void CanManager::ReceiveUnsubscribe(std::string_view id)
    {
        std::lock_guard lock(m_receive_subs_mutex);
        m_receive_subs.erase(std::string(id));
    }

    void CanManager::receiveAction(const RxPacket& msg)
    {
        std::lock_guard lock(m_receive_subs_mutex);
        for (auto& sub : m_receive_subs)
        {
            sub.second(msg);
        }
    }

    //
    // State
    //

    CanManagerState CanManager::StateGet() const
    {
        return m_state;
    }

    void CanManager::stateSet(CanManagerState val)
    {
        m_state = val;
    }

    //
    // Thread
    //

    void CanManager::threadFunction()
    {
        m_thread_abort = false;

        while (!m_thread_abort)
        {
            threadFunctionOperating();
        }
    }

    void CanManager::threadFunctionOperating()
    {
        {
            std::unique_lock lock(m_thread_mtx);
            m_thread_cv.wait(lock);
        }

        bool again = true;
        while (again && !m_thread_abort)
        {
            again = false;

            // RX
            if (m_can_channel)
            {
                auto* msg = uni_can_channel_receive(m_can_channel);
                if (msg)
                {
                    // Always publish raw CAN chunks (even in ProtoPlexer mode), so UI can show both:
                    //  - physical CAN chunks
                    //  - decoded ProtoPlexer messages
                    {
                        CanMessagePtr ptr(uni_can_message_clone(msg), &uni_can_message_free);
                        if (ptr)
                        {
                            receiveAction(ptr);
                        }
                    }

                    if (m_backend_active == ParseBackend::ProtoPlexer)
                    {
                        uni_can_protoplexer_msg_t* out_msg = nullptr;
                        const auto res = uni_can_protoplexer_add_chunk(&m_pp_ctx, msg, &out_msg);
                        (void)res;
                        if (out_msg)
                        {
                            ProtoPlexerMessage rx{};
                            rx.message_id = out_msg->message_id;
                            rx.address_from = out_msg->address_from;
                            rx.address_to = out_msg->address_to;
                            rx.priority_inverted = out_msg->priority_inverted;
                            rx.data.assign(out_msg->data, out_msg->data + out_msg->length);
                            receiveAction(rx);
                            uni_can_protoplexer_msg_free(out_msg);
                        }
                    }

                    uni_can_message_free(msg);
                    again = true;
                }
            }

            // TX
            {
                std::lock_guard<std::mutex> lock(m_tx_queue_mtx);
                while (!m_tx_queue.empty() && m_can_channel)
                {
                    TxPacket item = std::move(m_tx_queue.front());
                    m_tx_queue.pop();

                    if (std::holds_alternative<CanMessagePtr>(item))
                    {
                        auto ptr = std::get<CanMessagePtr>(item);
                        if (!ptr)
                        {
                            continue;
                        }

                        uni_can_channel_transmit(m_can_channel, ptr.get());
                        transmitAction(*ptr);
                        again = true;
                    }
                    else if (std::holds_alternative<ProtoPlexerMessage>(item))
                    {
                        if (m_backend_active != ParseBackend::ProtoPlexer)
                        {
                            continue;
                        }

                        const auto& pm = std::get<ProtoPlexerMessage>(item);
                        uni_can_protoplexer_msg_t* tx_msg = uni_can_protoplexer_msg_create(
                            pm.message_id,
                            pm.address_from,
                            pm.address_to,
                            pm.priority_inverted,
                            pm.data.data(),
                            static_cast<uint16_t>(pm.data.size()));
                        if (!tx_msg)
                        {
                            continue;
                        }

                        uni_can_message_t* chunks = nullptr;
                        size_t chunks_count = 0;
                        const auto build_res = uni_can_protoplexer_build_chunks(
                            tx_msg,
                            m_pp_cfg_active.max_chunk_length,
                            &chunks,
                            &chunks_count);

                        if (build_res == UNI_CAN_PROTOPLEXER_OK && chunks)
                        {
                            for (size_t i = 0; i < chunks_count; i++)
                            {
                                uni_can_channel_transmit(m_can_channel, &chunks[i]);
                                transmitAction(chunks[i]);
                            }
                            again = true;
                        }

                        uni_can_protoplexer_free_chunks(chunks);
                        uni_can_protoplexer_msg_free(tx_msg);
                    }
                }
            }
        }
    }

    void CanManager::ThreadNotify()
    {
        m_thread_cv.notify_all();
    }

    //
    // Transmit
    //

    uint32_t& CanManager::TransmitThrottle()
    {
        return m_transmit_throttle;
    }

    void CanManager::transmitAction(uni_can_message_t& /*msg*/)
    {
        if (m_transmit_throttle > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_transmit_throttle));
        }
    }
}
