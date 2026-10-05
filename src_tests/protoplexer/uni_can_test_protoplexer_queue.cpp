//
// Tests for uni_can_protoplexer_queue
//

// stdlib
#include <cstdint>
#include <cstring>
#include <deque>
#include <vector>

// catch2
#include <catch2/catch_test_macros.hpp>

// uni.can
#include "uni_can_protoplexer_queue.h"

namespace {
    constexpr std::uint16_t k_node_a = 0x001;
    constexpr std::uint16_t k_node_b = 0x011;

    struct Received {
        std::uint16_t message_id;
        std::uint16_t address_from;
        std::vector<std::uint8_t> data;
    };

    // One end of a network: what it sends goes to the wire of its peer
    struct Node {
        uni_can_protoplexer_queue_t queue{};
        std::deque<uni_can_message_t> wire_in;
        Node *peer{};

        std::vector<Received> received;

        // what the network says to the next chunks
        std::deque<uni_can_protoplexer_hw_result_t> hw_results;
        int send_calls{};

        static uni_can_protoplexer_hw_result_t send_chunk(void *cookie, const uni_can_message_t *chunk) {
            auto *self = static_cast<Node *>(cookie);
            self->send_calls++;

            auto result = UNI_CAN_PROTOPLEXER_HW_OK;
            if (!self->hw_results.empty()) {
                result = self->hw_results.front();
                self->hw_results.pop_front();
            }
            if (result == UNI_CAN_PROTOPLEXER_HW_OK && self->peer) {
                self->peer->wire_in.push_back(*chunk);
            }
            return result;
        }

        static bool receive_chunk(void *cookie, uni_can_message_t *chunk) {
            auto *self = static_cast<Node *>(cookie);
            if (self->wire_in.empty()) {
                return false;
            }
            *chunk = self->wire_in.front();
            self->wire_in.pop_front();
            return true;
        }

        static void rx_event(void *cookie, const uni_can_protoplexer_msg_t *msg) {
            auto *self = static_cast<Node *>(cookie);
            self->received.push_back({msg->message_id, msg->address_from, std::vector<std::uint8_t>(msg->data, msg->data + msg->length)});
        }

        bool init(std::uint16_t address, const uni_can_protoplexer_queue_storage_t *storage, std::uint32_t send_attempts = 3) {
            uni_can_protoplexer_queue_config_t cfg{};
            cfg.own_address = address;
            cfg.max_chunk_length = 8;
            cfg.send_attempts = send_attempts;
            cfg.storage = storage;
            cfg.send_chunk = &Node::send_chunk;
            cfg.receive_chunk = &Node::receive_chunk;
            cfg.rx_event = &Node::rx_event;
            cfg.cookie = this;
            return uni_can_protoplexer_queue_init(&queue, &cfg);
        }

        void pump() {
            while (uni_can_protoplexer_queue_poll_tx(&queue) == UNI_CAN_PROTOPLEXER_OK) {
            }
        }

        void drain() {
            while (uni_can_protoplexer_queue_poll_rx(&queue) != UNI_CAN_PROTOPLEXER_POLLING_RX_HW_RX_EMPTY) {
            }
            while (uni_can_protoplexer_queue_poll_new(&queue) == UNI_CAN_PROTOPLEXER_OK) {
            }
        }
    };

    // 4 reassemblies, 24 bytes of data, 2 messages in the inbox, 6 chunks in the outbox
    UNI_CAN_PROTOPLEXER_QUEUE_STORAGE_DEFINITION(g_storage_a, 4, 24, 2, 6);
    UNI_CAN_PROTOPLEXER_QUEUE_STORAGE_DEFINITION(g_storage_b, 4, 24, 2, 6);
}

TEST_CASE("protoplexer_queue_init_checks_storage", "protoplexer_queue") {
    Node node;

    uni_can_protoplexer_queue_config_t cfg{};
    cfg.own_address = k_node_a;
    cfg.max_chunk_length = 8;
    REQUIRE_FALSE(uni_can_protoplexer_queue_init(&node.queue, &cfg));

    uni_can_protoplexer_queue_storage_t broken = g_storage_a;
    broken.outbox_size = 0;
    cfg.storage = &broken;
    REQUIRE_FALSE(uni_can_protoplexer_queue_init(&node.queue, &cfg));

    broken = g_storage_a;
    broken.inbox_payload = nullptr;
    REQUIRE_FALSE(uni_can_protoplexer_queue_init(&node.queue, &cfg));

    cfg.storage = &g_storage_a;
    cfg.max_chunk_length = 5;
    REQUIRE_FALSE(uni_can_protoplexer_queue_init(&node.queue, &cfg));

    cfg.max_chunk_length = 8;
    REQUIRE(uni_can_protoplexer_queue_init(&node.queue, &cfg));

    // a queue that was not set up does nothing
    uni_can_protoplexer_queue_t idle{};
    REQUIRE(uni_can_protoplexer_queue_send(&idle, 1, k_node_a, k_node_b, 0, nullptr, 0) == UNI_CAN_PROTOPLEXER_FAILURE);
    REQUIRE(uni_can_protoplexer_queue_poll_tx(&idle) == UNI_CAN_PROTOPLEXER_POLLING_TX_BUFFER_EMPTY);
    REQUIRE(uni_can_protoplexer_queue_poll_rx(&idle) == UNI_CAN_PROTOPLEXER_POLLING_RX_HW_RX_EMPTY);
    REQUIRE(uni_can_protoplexer_queue_poll_new(&idle) == UNI_CAN_PROTOPLEXER_POLLING_NEW_NO_NEW_MESSAGES);
}

TEST_CASE("protoplexer_queue_roundtrip", "protoplexer_queue") {
    Node a;
    Node b;
    REQUIRE(a.init(k_node_a, &g_storage_a));
    REQUIRE(b.init(k_node_b, &g_storage_b));
    a.peer = &b;
    b.peer = &a;

    std::vector<std::uint8_t> data(24);
    for (std::size_t i = 0; i < data.size(); i++) {
        data[i] = static_cast<std::uint8_t>(0x30 + i);
    }

    // 6 + 24 bytes: 4 chunks
    REQUIRE(uni_can_protoplexer_queue_send(&a.queue, 0x0202, k_node_a, k_node_b, UNI_CAN_PROTOPLEXER_PRIORITY_MINIMAL, data.data(), 24) == UNI_CAN_PROTOPLEXER_OK);
    REQUIRE(a.queue.outbox_count == 4);

    // an empty message takes one more
    REQUIRE(uni_can_protoplexer_queue_send(&a.queue, 0x0113, k_node_a, k_node_b, UNI_CAN_PROTOPLEXER_PRIORITY_MINIMAL, nullptr, 0) == UNI_CAN_PROTOPLEXER_OK);
    REQUIRE(a.queue.outbox_count == 5);

    a.pump();
    REQUIRE(a.queue.outbox_count == 0);
    REQUIRE(uni_can_protoplexer_queue_poll_tx(&a.queue) == UNI_CAN_PROTOPLEXER_POLLING_TX_BUFFER_EMPTY);
    REQUIRE(b.wire_in.size() == 5);
    REQUIRE(uni_can_protoplexer_canid_get_is_first(b.wire_in[0].id));
    REQUIRE_FALSE(uni_can_protoplexer_canid_get_is_first(b.wire_in[1].id));
    REQUIRE(uni_can_protoplexer_canid_get_is_first(b.wire_in[4].id));

    b.drain();
    REQUIRE(b.received.size() == 2);
    REQUIRE(b.received[0].message_id == 0x0202);
    REQUIRE(b.received[0].address_from == k_node_a);
    REQUIRE(b.received[0].data == data);
    REQUIRE(b.received[1].message_id == 0x0113);
    REQUIRE(b.received[1].data.empty());
    REQUIRE(uni_can_protoplexer_queue_poll_new(&b.queue) == UNI_CAN_PROTOPLEXER_POLLING_NEW_NO_NEW_MESSAGES);
}

TEST_CASE("protoplexer_queue_send_limits", "protoplexer_queue") {
    Node a;
    REQUIRE(a.init(k_node_a, &g_storage_a));

    std::vector<std::uint8_t> data(25, 0x77);

    // longer than the storage is made for, or no data where there should be some
    REQUIRE(uni_can_protoplexer_queue_send(&a.queue, 0x0202, k_node_a, k_node_b, 0, data.data(), 25) == UNI_CAN_PROTOPLEXER_SEND_MSG_INVALID_SIZE);
    REQUIRE(uni_can_protoplexer_queue_send(&a.queue, 0x0202, k_node_a, k_node_b, 0, nullptr, 4) == UNI_CAN_PROTOPLEXER_SEND_MSG_INVALID_SIZE);

    // the outbox takes 6 chunks: a message of 4 fits once, the second is refused as a whole
    REQUIRE(uni_can_protoplexer_queue_send(&a.queue, 0x0202, k_node_a, k_node_b, 0, data.data(), 24) == UNI_CAN_PROTOPLEXER_OK);
    REQUIRE(uni_can_protoplexer_queue_send(&a.queue, 0x0203, k_node_a, k_node_b, 0, data.data(), 24) == UNI_CAN_PROTOPLEXER_SEND_MSG_CANT_ADD_MESSAGE_CHUNKS);
    REQUIRE(a.queue.outbox_count == 4);

    // two short ones still fit
    REQUIRE(uni_can_protoplexer_queue_send(&a.queue, 0x0204, k_node_a, k_node_b, 0, data.data(), 2) == UNI_CAN_PROTOPLEXER_OK);
    REQUIRE(uni_can_protoplexer_queue_send(&a.queue, 0x0205, k_node_a, k_node_b, 0, data.data(), 2) == UNI_CAN_PROTOPLEXER_OK);
    REQUIRE(uni_can_protoplexer_queue_send(&a.queue, 0x0206, k_node_a, k_node_b, 0, nullptr, 0) == UNI_CAN_PROTOPLEXER_SEND_MSG_CANT_ADD_MESSAGE_CHUNKS);
}

TEST_CASE("protoplexer_queue_outbox_wraps", "protoplexer_queue") {
    Node a;
    Node b;
    REQUIRE(a.init(k_node_a, &g_storage_a));
    REQUIRE(b.init(k_node_b, &g_storage_b));
    a.peer = &b;

    // many messages through an outbox of 6 chunks and an inbox of 2 messages
    for (std::uint8_t round = 0; round < 20; round++) {
        std::vector<std::uint8_t> data(10, round);
        REQUIRE(uni_can_protoplexer_queue_send(&a.queue, static_cast<std::uint16_t>(0x0300 + round), k_node_a, k_node_b, 0, data.data(), 10) == UNI_CAN_PROTOPLEXER_OK);
        a.pump();
        b.drain();
        REQUIRE(b.received.size() == static_cast<std::size_t>(round) + 1);
        REQUIRE(b.received.back().message_id == 0x0300 + round);
        REQUIRE(b.received.back().data == data);
    }
}

TEST_CASE("protoplexer_queue_resend_and_drop", "protoplexer_queue") {
    Node a;
    Node b;
    REQUIRE(a.init(k_node_a, &g_storage_a, 3));
    REQUIRE(b.init(k_node_b, &g_storage_b));
    a.peer = &b;

    std::vector<std::uint8_t> data(2, 0x42);
    REQUIRE(uni_can_protoplexer_queue_send(&a.queue, 0x0401, k_node_a, k_node_b, 0, data.data(), 2) == UNI_CAN_PROTOPLEXER_OK);

    // the network asks for the chunk again: it stays, and the fourth time in a row is reported
    a.hw_results = {UNI_CAN_PROTOPLEXER_HW_ERROR_RESEND_REQUIRED, UNI_CAN_PROTOPLEXER_HW_ERROR_RESEND_REQUIRED,
                    UNI_CAN_PROTOPLEXER_HW_ERROR_RESEND_REQUIRED, UNI_CAN_PROTOPLEXER_HW_ERROR_RESEND_REQUIRED};
    REQUIRE(uni_can_protoplexer_queue_poll_tx(&a.queue) == UNI_CAN_PROTOPLEXER_OK);
    REQUIRE(uni_can_protoplexer_queue_poll_tx(&a.queue) == UNI_CAN_PROTOPLEXER_OK);
    REQUIRE(uni_can_protoplexer_queue_poll_tx(&a.queue) == UNI_CAN_PROTOPLEXER_OK);
    REQUIRE(uni_can_protoplexer_queue_poll_tx(&a.queue) == UNI_CAN_PROTOPLEXER_HW_SEND_TOO_MANY_ATTEMPTS);
    REQUIRE(a.queue.outbox_count == 1);
    REQUIRE(b.wire_in.empty());

    // then it goes out
    REQUIRE(uni_can_protoplexer_queue_poll_tx(&a.queue) == UNI_CAN_PROTOPLEXER_OK);
    REQUIRE(a.queue.outbox_count == 0);
    REQUIRE(b.wire_in.size() == 1);

    // a chunk the network gives up on is dropped
    REQUIRE(uni_can_protoplexer_queue_send(&a.queue, 0x0402, k_node_a, k_node_b, 0, data.data(), 2) == UNI_CAN_PROTOPLEXER_OK);
    a.hw_results = {UNI_CAN_PROTOPLEXER_HW_ERROR_NON_RECOVERABLE};
    REQUIRE(uni_can_protoplexer_queue_poll_tx(&a.queue) == UNI_CAN_PROTOPLEXER_HW_SEND_NON_RECOVERABLE);
    REQUIRE(a.queue.outbox_count == 0);
    REQUIRE(b.wire_in.size() == 1);
}

TEST_CASE("protoplexer_queue_inbox_full", "protoplexer_queue") {
    Node a;
    Node b;
    REQUIRE(a.init(k_node_a, &g_storage_a));
    REQUIRE(b.init(k_node_b, &g_storage_b));
    a.peer = &b;

    // three messages for an inbox of two: the third is lost, with its result saying so
    for (std::uint8_t idx = 0; idx < 3; idx++) {
        REQUIRE(uni_can_protoplexer_queue_send(&a.queue, static_cast<std::uint16_t>(0x0500 + idx), k_node_a, k_node_b, 0, &idx, 1) == UNI_CAN_PROTOPLEXER_OK);
    }
    a.pump();

    REQUIRE(uni_can_protoplexer_queue_poll_rx(&b.queue) == UNI_CAN_PROTOPLEXER_OK);
    REQUIRE(uni_can_protoplexer_queue_poll_rx(&b.queue) == UNI_CAN_PROTOPLEXER_OK);
    REQUIRE(uni_can_protoplexer_queue_poll_rx(&b.queue) == UNI_CAN_PROTOPLEXER_ADDING_CHUNK_CANT_SAVE_MESSAGE);
    REQUIRE(uni_can_protoplexer_queue_poll_rx(&b.queue) == UNI_CAN_PROTOPLEXER_POLLING_RX_HW_RX_EMPTY);

    while (uni_can_protoplexer_queue_poll_new(&b.queue) == UNI_CAN_PROTOPLEXER_OK) {
    }
    REQUIRE(b.received.size() == 2);
    REQUIRE(b.received[0].message_id == 0x0500);
    REQUIRE(b.received[0].data == std::vector<std::uint8_t>{0});
    REQUIRE(b.received[1].message_id == 0x0501);
    REQUIRE(b.received[1].data == std::vector<std::uint8_t>{1});

    // and the inbox takes messages again
    std::uint8_t value = 9;
    REQUIRE(uni_can_protoplexer_queue_send(&a.queue, 0x0509, k_node_a, k_node_b, 0, &value, 1) == UNI_CAN_PROTOPLEXER_OK);
    a.pump();
    b.drain();
    REQUIRE(b.received.size() == 3);
    REQUIRE(b.received[2].message_id == 0x0509);
}

TEST_CASE("protoplexer_queue_ignores_other_receivers", "protoplexer_queue") {
    Node a;
    Node b;
    REQUIRE(a.init(k_node_a, &g_storage_a));
    REQUIRE(b.init(k_node_b, &g_storage_b));
    a.peer = &b;

    std::uint8_t value = 1;
    REQUIRE(uni_can_protoplexer_queue_send(&a.queue, 0x0601, k_node_a, 0x012, 0, &value, 1) == UNI_CAN_PROTOPLEXER_OK);
    a.pump();

    REQUIRE(uni_can_protoplexer_queue_poll_rx(&b.queue) == UNI_CAN_PROTOPLEXER_ADDING_CHUNK_UNRELATED_ADDRESS_TO);
    REQUIRE(uni_can_protoplexer_queue_poll_new(&b.queue) == UNI_CAN_PROTOPLEXER_POLLING_NEW_NO_NEW_MESSAGES);
}

TEST_CASE("protoplexer_queue_send_from_rx_event", "protoplexer_queue") {
    // a handler that answers from inside rx_event, as a node that is asked for something does
    struct Responder : Node {
        static void answer(void *cookie, const uni_can_protoplexer_msg_t *msg) {
            auto *self = static_cast<Responder *>(cookie);
            Node::rx_event(cookie, msg);
            std::uint8_t reply = static_cast<std::uint8_t>(msg->data[0] + 1);
            REQUIRE(uni_can_protoplexer_queue_send(&self->queue, 0x0702, k_node_b, msg->address_from, 0, &reply, 1) == UNI_CAN_PROTOPLEXER_OK);
        }
    };

    Node a;
    Responder b;
    REQUIRE(a.init(k_node_a, &g_storage_a));
    REQUIRE(b.init(k_node_b, &g_storage_b));
    b.queue.config.rx_event = &Responder::answer;
    a.peer = &b;
    b.peer = &a;

    std::uint8_t value = 41;
    REQUIRE(uni_can_protoplexer_queue_send(&a.queue, 0x0701, k_node_a, k_node_b, 0, &value, 1) == UNI_CAN_PROTOPLEXER_OK);
    a.pump();
    b.drain();
    b.pump();
    a.drain();

    REQUIRE(a.received.size() == 1);
    REQUIRE(a.received[0].message_id == 0x0702);
    REQUIRE(a.received[0].data == std::vector<std::uint8_t>{42});
}
