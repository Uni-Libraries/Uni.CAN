//
// Tests for the uni_can_channel functions: they pass a call on to the backend of the channel
//

// stdlib
#include <cstdint>
#include <cstring>
#include <deque>

// catch2
#include <catch2/catch_test_macros.hpp>

// uni.can
#include "uni_can_channel.h"

namespace {
    // A backend that keeps what is sent and gives out what the test puts in
    struct FakeChannel {
        // first: the uni_can_channel_ functions find the operations here
        uni_can_channel_base_t base{};

        bool opened{};
        bool destroyed{};
        bool idle{true};
        int aborts{};
        std::deque<uni_can_message_t> sent;
        std::deque<uni_can_message_t> incoming;
        std::uint32_t filter_id{};
        std::uint32_t filter_mask{};
        uni_can_channel_receive_handler_f handler{};
        void *handler_cookie{};
    };

    FakeChannel *fake(void *channel) {
        return static_cast<FakeChannel *>(channel);
    }

    bool fake_init(void *) {
        return true;
    }

    bool fake_open(void *channel) {
        fake(channel)->opened = true;
        return true;
    }

    bool fake_close(void *channel) {
        fake(channel)->opened = false;
        return true;
    }

    bool fake_destroy(void *channel) {
        fake(channel)->destroyed = true;
        return true;
    }

    uni_can_message_t *fake_receive(void *channel) {
        auto *self = fake(channel);
        if (self->incoming.empty()) {
            return nullptr;
        }
        auto *msg = uni_can_message_clone(&self->incoming.front());
        self->incoming.pop_front();
        return msg;
    }

    bool fake_receive_to(void *channel, uni_can_message_t *msg) {
        auto *self = fake(channel);
        if (self->incoming.empty()) {
            return false;
        }
        *msg = self->incoming.front();
        self->incoming.pop_front();
        return true;
    }

    bool fake_transmit(void *channel, const uni_can_message_t *msg) {
        fake(channel)->sent.push_back(*msg);
        return true;
    }

    bool fake_transmit_idle(void *channel) {
        return fake(channel)->idle;
    }

    bool fake_transmit_abort(void *channel) {
        fake(channel)->aborts++;
        return true;
    }

    bool fake_set_receive_handler(void *channel, uni_can_channel_receive_handler_f func, void *cookie) {
        fake(channel)->handler = func;
        fake(channel)->handler_cookie = cookie;
        return true;
    }

    bool fake_filter_add(void *channel, std::uint32_t id, std::uint32_t mask) {
        fake(channel)->filter_id = id;
        fake(channel)->filter_mask = mask;
        return true;
    }

    uni_can_channel_ops_t full_ops() {
        uni_can_channel_ops_t ops{};
        ops.init = &fake_init;
        ops.open = &fake_open;
        ops.close = &fake_close;
        ops.destroy = &fake_destroy;
        ops.receive = &fake_receive;
        ops.receive_to = &fake_receive_to;
        ops.transmit = &fake_transmit;
        ops.transmit_idle = &fake_transmit_idle;
        ops.transmit_abort = &fake_transmit_abort;
        ops.set_receive_handler = &fake_set_receive_handler;
        ops.filter_add = &fake_filter_add;
        return ops;
    }

    uni_can_message_t message(std::uint32_t id, std::uint8_t value) {
        uni_can_message_t msg{};
        msg.id = id;
        msg.flags = UNI_CAN_MSG_FLAG_EXT_ID;
        msg.len = 2;
        msg.data.u8[0] = value;
        msg.data.u8[1] = static_cast<std::uint8_t>(~value);
        return msg;
    }

    void handler(void *, void *) {
    }
}

TEST_CASE("channel_null_is_refused", "channel") {
    uni_can_message_t msg{};
    REQUIRE_FALSE(uni_can_channel_init(nullptr));
    REQUIRE_FALSE(uni_can_channel_open(nullptr));
    REQUIRE_FALSE(uni_can_channel_close(nullptr));
    REQUIRE_FALSE(uni_can_channel_destroy(nullptr));
    REQUIRE(uni_can_channel_receive(nullptr) == nullptr);
    REQUIRE_FALSE(uni_can_channel_receive_to(nullptr, &msg));
    REQUIRE_FALSE(uni_can_channel_transmit(nullptr, &msg));
    REQUIRE_FALSE(uni_can_channel_transmit_idle(nullptr));
    REQUIRE_FALSE(uni_can_channel_transmit_abort(nullptr));
    REQUIRE_FALSE(uni_can_channel_set_receive_handler(nullptr, &handler, nullptr));
    REQUIRE_FALSE(uni_can_channel_filter_add(nullptr, 1, 1));
}

TEST_CASE("channel_missing_operations_are_refused", "channel") {
    const uni_can_channel_ops_t ops{};
    FakeChannel channel;
    channel.base.ops = &ops;

    uni_can_message_t msg{};
    REQUIRE_FALSE(uni_can_channel_init(&channel));
    REQUIRE_FALSE(uni_can_channel_open(&channel));
    REQUIRE_FALSE(uni_can_channel_close(&channel));
    REQUIRE_FALSE(uni_can_channel_destroy(&channel));
    REQUIRE(uni_can_channel_receive(&channel) == nullptr);
    REQUIRE_FALSE(uni_can_channel_receive_to(&channel, &msg));
    REQUIRE_FALSE(uni_can_channel_transmit(&channel, &msg));
    REQUIRE_FALSE(uni_can_channel_transmit_idle(&channel));
    REQUIRE_FALSE(uni_can_channel_transmit_abort(&channel));
    REQUIRE_FALSE(uni_can_channel_set_receive_handler(&channel, &handler, nullptr));
    REQUIRE_FALSE(uni_can_channel_filter_add(&channel, 1, 1));

    // a channel that has no operations at all, e.g. one that was destroyed
    channel.base.ops = nullptr;
    REQUIRE_FALSE(uni_can_channel_open(&channel));
    REQUIRE_FALSE(uni_can_channel_transmit(&channel, &msg));
}

TEST_CASE("channel_calls_reach_the_backend", "channel") {
    const uni_can_channel_ops_t ops = full_ops();
    FakeChannel channel;
    channel.base.ops = &ops;

    REQUIRE(uni_can_channel_init(&channel));
    REQUIRE(uni_can_channel_open(&channel));
    REQUIRE(channel.opened);

    const uni_can_message_t out = message(0x1E011001, 0x5A);
    REQUIRE(uni_can_channel_transmit(&channel, &out));
    REQUIRE_FALSE(uni_can_channel_transmit(&channel, nullptr));
    REQUIRE(channel.sent.size() == 1);
    REQUIRE(channel.sent[0].id == out.id);
    REQUIRE(channel.sent[0].data.u8[0] == 0x5A);

    REQUIRE(uni_can_channel_transmit_idle(&channel));
    channel.idle = false;
    REQUIRE_FALSE(uni_can_channel_transmit_idle(&channel));
    REQUIRE(uni_can_channel_transmit_abort(&channel));
    REQUIRE(channel.aborts == 1);

    int cookie = 0;
    REQUIRE(uni_can_channel_set_receive_handler(&channel, &handler, &cookie));
    REQUIRE(channel.handler == &handler);
    REQUIRE(channel.handler_cookie == &cookie);

    REQUIRE(uni_can_channel_filter_add(&channel, 0x001, 0x7FF));
    REQUIRE(channel.filter_id == 0x001);
    REQUIRE(channel.filter_mask == 0x7FF);

    REQUIRE(uni_can_channel_close(&channel));
    REQUIRE_FALSE(channel.opened);
    REQUIRE(uni_can_channel_destroy(&channel));
    REQUIRE(channel.destroyed);
}

TEST_CASE("channel_receive_both_ways", "channel") {
    // a backend that only fills in memory of the caller serves both ways of receiving
    for (int variant = 0; variant < 2; variant++) {
        uni_can_channel_ops_t ops = full_ops();
        if (variant == 1) {
            ops.receive = nullptr;
        }

        FakeChannel channel;
        channel.base.ops = &ops;
        channel.incoming.push_back(message(0x100, 1));
        channel.incoming.push_back(message(0x200, 2));

        uni_can_message_t *first = uni_can_channel_receive(&channel);
        REQUIRE(first != nullptr);
        REQUIRE(first->id == 0x100);
        REQUIRE(first->len == 2);
        REQUIRE(first->data.u8[0] == 1);
        uni_can_message_free(first);

        uni_can_message_t second{};
        REQUIRE(uni_can_channel_receive_to(&channel, &second));
        REQUIRE(second.id == 0x200);
        REQUIRE(second.len == 2);
        REQUIRE(second.data.u8[0] == 2);

        REQUIRE(uni_can_channel_receive(&channel) == nullptr);
        REQUIRE_FALSE(uni_can_channel_receive_to(&channel, &second));
        REQUIRE_FALSE(uni_can_channel_receive_to(&channel, nullptr));
    }
}

TEST_CASE("channel_receive_on_the_heap_only", "channel") {
    // a backend that only gives out objects on the heap cannot fill in memory of the caller
    uni_can_channel_ops_t ops = full_ops();
    ops.receive_to = nullptr;

    FakeChannel channel;
    channel.base.ops = &ops;
    channel.incoming.push_back(message(0x100, 1));

    uni_can_message_t msg{};
    REQUIRE_FALSE(uni_can_channel_receive_to(&channel, &msg));

    uni_can_message_t *first = uni_can_channel_receive(&channel);
    REQUIRE(first != nullptr);
    REQUIRE(first->id == 0x100);
    uni_can_message_free(first);
}
