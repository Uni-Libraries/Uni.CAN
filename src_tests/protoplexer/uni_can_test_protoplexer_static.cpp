//
// Tests for uni_can_protoplexer without the heap: memory of the application, single chunks
// and message views
//

// stdlib
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

// catch2
#include <catch2/catch_test_macros.hpp>

// uni.can
#include "uni_can_protoplexer.h"

namespace {
    constexpr std::uint16_t k_from = 0x011;
    constexpr std::uint16_t k_to = 0x001;
    constexpr std::uint16_t k_channels = 2;
    constexpr std::uint16_t k_payload = 16;

    struct StaticContext {
        std::array<uni_can_protoplexer_rx_buffer_t, k_channels> buffers{};
        std::array<std::uint8_t, k_channels * k_payload> payload{};
        uni_can_protoplexer_ctx_t ctx{};

        StaticContext() {
            uni_can_protoplexer_config_t cfg{};
            cfg.own_address = k_to;
            cfg.max_chunk_length = 8;
            cfg.max_channels = k_channels;
            cfg.max_payload = k_payload;
            cfg.rx_buffers = buffers.data();
            cfg.rx_payload = payload.data();
            REQUIRE(uni_can_protoplexer_init(&ctx, &cfg));
        }

        ~StaticContext() {
            uni_can_protoplexer_deinit(&ctx);
        }
    };

    std::vector<uni_can_message_t> chunks_of(std::uint16_t msg_id, std::uint16_t from, std::vector<std::uint8_t> &data) {
        uni_can_protoplexer_msg_t msg{};
        msg.message_id = msg_id;
        msg.address_from = from;
        msg.address_to = k_to;
        msg.priority_inverted = UNI_CAN_PROTOPLEXER_PRIORITY_MINIMAL;
        msg.length = static_cast<std::uint16_t>(data.size());
        msg.data = data.data();

        std::vector<uni_can_message_t> result(uni_can_protoplexer_chunks_count(msg.length, 8));
        for (std::size_t i = 0; i < result.size(); i++) {
            REQUIRE(uni_can_protoplexer_build_chunk(&msg, 8, i, &result[i]) == UNI_CAN_PROTOPLEXER_OK);
        }
        return result;
    }
}

TEST_CASE("protoplexer_chunks_count", "protoplexer") {
    REQUIRE(uni_can_protoplexer_chunks_count(0, 8) == 1);
    REQUIRE(uni_can_protoplexer_chunks_count(2, 8) == 1);
    REQUIRE(uni_can_protoplexer_chunks_count(3, 8) == 2);
    REQUIRE(uni_can_protoplexer_chunks_count(20, 8) == 4);
    REQUIRE(uni_can_protoplexer_chunks_count(0, 6) == 1);
    REQUIRE(uni_can_protoplexer_chunks_count(1, 6) == 2);

    // a chunk that cannot hold the header, or is no CAN frame
    REQUIRE(uni_can_protoplexer_chunks_count(0, 5) == 0);
    REQUIRE(uni_can_protoplexer_chunks_count(0, 9) == 0);
}

TEST_CASE("protoplexer_build_chunk_matches_build_chunks", "protoplexer") {
    std::vector<std::uint8_t> data(37);
    for (std::size_t i = 0; i < data.size(); i++) {
        data[i] = static_cast<std::uint8_t>(0xA0 + i);
    }

    for (std::uint16_t mtu = 6; mtu <= 8; mtu++) {
        for (std::size_t length : {std::size_t{0}, std::size_t{1}, std::size_t{2}, std::size_t{3}, std::size_t{8}, data.size()}) {
            uni_can_protoplexer_msg_t msg{};
            msg.message_id = 0x0202;
            msg.address_from = k_from;
            msg.address_to = k_to;
            msg.priority_inverted = 0x03;
            msg.length = static_cast<std::uint16_t>(length);
            msg.data = data.data();

            uni_can_message_t *all = nullptr;
            std::size_t all_count = 0;
            REQUIRE(uni_can_protoplexer_build_chunks(&msg, mtu, &all, &all_count) == UNI_CAN_PROTOPLEXER_OK);
            REQUIRE(all_count == uni_can_protoplexer_chunks_count(msg.length, mtu));

            for (std::size_t i = 0; i < all_count; i++) {
                uni_can_message_t one{};
                REQUIRE(uni_can_protoplexer_build_chunk(&msg, mtu, i, &one) == UNI_CAN_PROTOPLEXER_OK);
                REQUIRE(one.id == all[i].id);
                REQUIRE(one.flags == all[i].flags);
                REQUIRE(one.len == all[i].len);
                REQUIRE(std::memcmp(one.data.u8, all[i].data.u8, one.len) == 0);
            }

            uni_can_message_t past{};
            REQUIRE(uni_can_protoplexer_build_chunk(&msg, mtu, all_count, &past) == UNI_CAN_PROTOPLEXER_FAILURE);

            uni_can_protoplexer_free_chunks(all);
        }
    }
}

TEST_CASE("protoplexer_build_chunk_rejects_missing_data", "protoplexer") {
    uni_can_protoplexer_msg_t msg{};
    msg.message_id = 0x0202;
    msg.address_from = k_from;
    msg.address_to = k_to;
    msg.length = 4;
    msg.data = nullptr;

    uni_can_message_t chunk{};
    REQUIRE(uni_can_protoplexer_build_chunk(&msg, 8, 0, &chunk) == UNI_CAN_PROTOPLEXER_SEND_MSG_INVALID_SIZE);
    REQUIRE(uni_can_protoplexer_build_chunk(&msg, 5, 0, &chunk) == UNI_CAN_PROTOPLEXER_SEND_MSG_HW_CANT_SEND_HEADER);
}

TEST_CASE("protoplexer_static_init_checks_memory", "protoplexer") {
    std::array<uni_can_protoplexer_rx_buffer_t, 2> buffers{};
    std::array<std::uint8_t, 32> payload{};

    uni_can_protoplexer_config_t cfg{};
    cfg.own_address = k_to;
    cfg.max_chunk_length = 8;
    cfg.rx_buffers = buffers.data();
    cfg.rx_payload = payload.data();
    cfg.max_payload = 16;

    uni_can_protoplexer_ctx_t ctx{};

    // the number of reassemblies is not known
    cfg.max_channels = 0;
    REQUIRE_FALSE(uni_can_protoplexer_init(&ctx, &cfg));

    // no memory for the data
    cfg.max_channels = 2;
    cfg.rx_payload = nullptr;
    REQUIRE_FALSE(uni_can_protoplexer_init(&ctx, &cfg));

    cfg.rx_payload = payload.data();
    REQUIRE(uni_can_protoplexer_init(&ctx, &cfg));
    REQUIRE(ctx.rx == buffers.data());
    uni_can_protoplexer_deinit(&ctx);

    // the function for memory of the application does not fall back to the heap
    REQUIRE(uni_can_protoplexer_init_static(&ctx, &cfg));
    REQUIRE(ctx.rx == buffers.data());
    uni_can_protoplexer_deinit(&ctx);

    cfg.rx_buffers = nullptr;
    REQUIRE_FALSE(uni_can_protoplexer_init_static(&ctx, &cfg));
}

TEST_CASE("protoplexer_static_view_reassembles", "protoplexer") {
    StaticContext s;

    std::vector<std::uint8_t> data(k_payload);
    for (std::size_t i = 0; i < data.size(); i++) {
        data[i] = static_cast<std::uint8_t>(i * 3);
    }
    auto chunks = chunks_of(0x0110, k_from, data);
    REQUIRE(chunks.size() == 3);

    uni_can_protoplexer_msg_t msg{};
    bool complete = true;
    for (std::size_t i = 0; i + 1 < chunks.size(); i++) {
        REQUIRE(uni_can_protoplexer_add_chunk_view(&s.ctx, &chunks[i], &msg, &complete) == UNI_CAN_PROTOPLEXER_OK);
        REQUIRE_FALSE(complete);
    }
    REQUIRE(uni_can_protoplexer_add_chunk_view(&s.ctx, &chunks.back(), &msg, &complete) == UNI_CAN_PROTOPLEXER_OK);
    REQUIRE(complete);

    REQUIRE(msg.message_id == 0x0110);
    REQUIRE(msg.address_from == k_from);
    REQUIRE(msg.address_to == k_to);
    REQUIRE(msg.priority_inverted == UNI_CAN_PROTOPLEXER_PRIORITY_MINIMAL);
    REQUIRE(msg.length == data.size());
    REQUIRE(std::memcmp(msg.data, data.data(), data.size()) == 0);

    // the data is in the memory the application gave
    REQUIRE(msg.data >= s.payload.data());
    REQUIRE(msg.data < s.payload.data() + s.payload.size());
}

TEST_CASE("protoplexer_static_view_empty_message", "protoplexer") {
    StaticContext s;

    std::vector<std::uint8_t> data;
    auto chunks = chunks_of(0x0113, k_from, data);
    REQUIRE(chunks.size() == 1);

    uni_can_protoplexer_msg_t msg{};
    bool complete = false;
    REQUIRE(uni_can_protoplexer_add_chunk_view(&s.ctx, &chunks[0], &msg, &complete) == UNI_CAN_PROTOPLEXER_OK);
    REQUIRE(complete);
    REQUIRE(msg.message_id == 0x0113);
    REQUIRE(msg.length == 0);
}

TEST_CASE("protoplexer_static_refuses_long_message", "protoplexer") {
    StaticContext s;

    std::vector<std::uint8_t> data(k_payload + 1, 0x55);
    auto chunks = chunks_of(0x0110, k_from, data);

    uni_can_protoplexer_msg_t msg{};
    bool complete = false;
    REQUIRE(uni_can_protoplexer_add_chunk_view(&s.ctx, &chunks[0], &msg, &complete) == UNI_CAN_PROTOPLEXER_ADDING_CHUNK_CANT_ALLOCATE_DATA);
    REQUIRE_FALSE(complete);

    // nothing is left of it: its data chunks have no header to belong to
    REQUIRE(uni_can_protoplexer_add_chunk_view(&s.ctx, &chunks[1], &msg, &complete) == UNI_CAN_PROTOPLEXER_ADDING_CHUNK_DATA_WITHOUT_HEADER);
}

TEST_CASE("protoplexer_static_reassembly_is_reused", "protoplexer") {
    StaticContext s;

    // more messages than there are reassemblies, one after another and from different senders
    for (std::uint16_t round = 0; round < 10; round++) {
        std::vector<std::uint8_t> data(k_payload, static_cast<std::uint8_t>(round));
        auto chunks = chunks_of(0x0200 + round, static_cast<std::uint16_t>(k_from + round), data);

        uni_can_protoplexer_msg_t msg{};
        bool complete = false;
        for (auto &chunk : chunks) {
            REQUIRE(uni_can_protoplexer_add_chunk_view(&s.ctx, &chunk, &msg, &complete) == UNI_CAN_PROTOPLEXER_OK);
        }
        REQUIRE(complete);
        REQUIRE(msg.message_id == 0x0200 + round);
        REQUIRE(msg.data[k_payload - 1] == round);
    }
}

TEST_CASE("protoplexer_static_interleaved_senders", "protoplexer") {
    StaticContext s;

    std::vector<std::uint8_t> data_a(k_payload, 0xAA);
    std::vector<std::uint8_t> data_b(k_payload, 0xBB);
    auto chunks_a = chunks_of(0x0301, 0x021, data_a);
    auto chunks_b = chunks_of(0x0302, 0x022, data_b);
    REQUIRE(chunks_a.size() == chunks_b.size());

    uni_can_protoplexer_msg_t msg{};
    bool complete = false;
    for (std::size_t i = 0; i < chunks_a.size(); i++) {
        REQUIRE(uni_can_protoplexer_add_chunk_view(&s.ctx, &chunks_a[i], &msg, &complete) == UNI_CAN_PROTOPLEXER_OK);
        if (i + 1 == chunks_a.size()) {
            REQUIRE(complete);
            REQUIRE(msg.message_id == 0x0301);
            REQUIRE(msg.data[0] == 0xAA);
        }

        REQUIRE(uni_can_protoplexer_add_chunk_view(&s.ctx, &chunks_b[i], &msg, &complete) == UNI_CAN_PROTOPLEXER_OK);
        if (i + 1 == chunks_b.size()) {
            REQUIRE(complete);
            REQUIRE(msg.message_id == 0x0302);
            REQUIRE(msg.data[0] == 0xBB);
        }
    }

    // both reassemblies are taken by senders that never finish: a third one finds no room
    std::vector<std::uint8_t> data(k_payload, 0x11);
    auto first = chunks_of(0x0303, 0x023, data);
    auto second = chunks_of(0x0304, 0x024, data);
    auto third = chunks_of(0x0305, 0x025, data);
    REQUIRE(uni_can_protoplexer_add_chunk_view(&s.ctx, &first[0], &msg, &complete) == UNI_CAN_PROTOPLEXER_OK);
    REQUIRE(uni_can_protoplexer_add_chunk_view(&s.ctx, &second[0], &msg, &complete) == UNI_CAN_PROTOPLEXER_OK);
    REQUIRE(uni_can_protoplexer_add_chunk_view(&s.ctx, &third[0], &msg, &complete) == UNI_CAN_PROTOPLEXER_ADDING_CHUNK_CANT_ALLOCATE_NEW_MSG);
}

TEST_CASE("protoplexer_view_with_heap", "protoplexer") {
    uni_can_protoplexer_config_t cfg{};
    cfg.own_address = k_to;
    cfg.max_chunk_length = 8;
    cfg.max_channels = 4;

    uni_can_protoplexer_ctx_t ctx{};
    REQUIRE(uni_can_protoplexer_init(&ctx, &cfg));

    std::vector<std::uint8_t> data(100);
    for (std::size_t i = 0; i < data.size(); i++) {
        data[i] = static_cast<std::uint8_t>(i);
    }

    // the view of one message is released when the next chunk comes; the last one by deinit
    for (int round = 0; round < 3; round++) {
        auto chunks = chunks_of(0x0400, k_from, data);
        uni_can_protoplexer_msg_t msg{};
        bool complete = false;
        for (auto &chunk : chunks) {
            REQUIRE(uni_can_protoplexer_add_chunk_view(&ctx, &chunk, &msg, &complete) == UNI_CAN_PROTOPLEXER_OK);
        }
        REQUIRE(complete);
        REQUIRE(msg.length == data.size());
        REQUIRE(std::memcmp(msg.data, data.data(), data.size()) == 0);
    }

    uni_can_protoplexer_deinit(&ctx);
}

TEST_CASE("protoplexer_heap_payload_limit", "protoplexer") {
    uni_can_protoplexer_config_t cfg{};
    cfg.own_address = k_to;
    cfg.max_chunk_length = 8;
    cfg.max_payload = 10;

    uni_can_protoplexer_ctx_t ctx{};
    REQUIRE(uni_can_protoplexer_init(&ctx, &cfg));

    std::vector<std::uint8_t> fits(10, 0x01);
    std::vector<std::uint8_t> too_long(11, 0x02);

    uni_can_protoplexer_msg_t *rx = nullptr;
    auto chunks = chunks_of(0x0500, k_from, too_long);
    REQUIRE(uni_can_protoplexer_add_chunk(&ctx, &chunks[0], &rx) == UNI_CAN_PROTOPLEXER_ADDING_CHUNK_CANT_ALLOCATE_DATA);
    REQUIRE(rx == nullptr);

    chunks = chunks_of(0x0501, k_from, fits);
    for (auto &chunk : chunks) {
        REQUIRE(uni_can_protoplexer_add_chunk(&ctx, &chunk, &rx) == UNI_CAN_PROTOPLEXER_OK);
    }
    REQUIRE(rx != nullptr);
    REQUIRE(rx->length == fits.size());
    uni_can_protoplexer_msg_free(rx);

    uni_can_protoplexer_deinit(&ctx);
}

TEST_CASE("protoplexer_rejects_oversized_frame", "protoplexer") {
    StaticContext s;

    uni_can_message_t chunk{};
    chunk.id = uni_can_protoplexer_canid_create(k_from, k_to, UNI_CAN_PROTOPLEXER_PRIORITY_MINIMAL, true);
    chunk.flags = UNI_CAN_MSG_FLAG_EXT_ID;
    chunk.len = 9;

    uni_can_protoplexer_msg_t msg{};
    bool complete = false;
    REQUIRE(uni_can_protoplexer_add_chunk_view(&s.ctx, &chunk, &msg, &complete) == UNI_CAN_PROTOPLEXER_ADDING_CHUNK_INVALID_CHUNK);
}
