//
// Tests for uni_can_protoplexer (C implementation)
//

// stdlib
#include <array>
#include <cstdint>

// catch2
#include <catch2/catch_test_macros.hpp>

// uni.can
#include "uni_can_protoplexer.h"

TEST_CASE("protoplexer_crc16_known_vectors", "protoplexer") {
    // From PROTOPLEXER_PROTOCOL_SPEC.md examples
    std::array<std::uint8_t, 1> payload0{0x00};
    REQUIRE(uni_can_protoplexer_crc16(payload0.data(), payload0.size()) == 0x0000);

    std::array<std::uint8_t, 1> payload1{0x01};
    REQUIRE(uni_can_protoplexer_crc16(payload1.data(), payload1.size()) == 0x1021);
}

TEST_CASE("protoplexer_canid_mapping", "protoplexer") {
    // Example A from PROTOPLEXER_PROTOCOL_SPEC.md
    const std::uint32_t can_id = uni_can_protoplexer_canid_create(0x011, 0x001, 0x01, true);
    REQUIRE(can_id == 0x03011001U);

    REQUIRE(uni_can_protoplexer_canid_get_from(can_id) == 0x011);
    REQUIRE(uni_can_protoplexer_canid_get_to(can_id) == 0x001);
    REQUIRE(uni_can_protoplexer_canid_get_priority_inverted(can_id) == 0x01);
    REQUIRE(uni_can_protoplexer_canid_get_is_first(can_id));
}

TEST_CASE("protoplexer_fragment_and_reassemble", "protoplexer") {
    constexpr std::uint16_t from = 0x011;
    constexpr std::uint16_t to = 0x001;
    constexpr std::uint16_t msg_id = 0x0110;
    constexpr std::uint8_t prio = 0x01;

    std::array<std::uint8_t, 20> payload{};
    for (std::size_t i = 0; i < payload.size(); i++) {
        payload[i] = static_cast<std::uint8_t>(i);
    }

    uni_can_protoplexer_msg_t *msg = uni_can_protoplexer_msg_create(msg_id, from, to, prio, payload.data(), payload.size());
    REQUIRE(msg != nullptr);

    uni_can_message_t *chunks = nullptr;
    std::size_t chunks_count = 0;
    REQUIRE(uni_can_protoplexer_build_chunks(msg, 8, &chunks, &chunks_count) == UNI_CAN_PROTOPLEXER_OK);
    REQUIRE(chunks != nullptr);
    REQUIRE(chunks_count == 4);

    // H=6, MTU=8, L=20 => chunks: 8, 8, 8, 2
    REQUIRE(chunks[0].len == 8);
    REQUIRE(uni_can_protoplexer_canid_get_is_first(chunks[0].id));
    REQUIRE(chunks[1].len == 8);
    REQUIRE_FALSE(uni_can_protoplexer_canid_get_is_first(chunks[1].id));
    REQUIRE(chunks[2].len == 8);
    REQUIRE(chunks[3].len == 2);

    uni_can_protoplexer_ctx_t ctx{};
    uni_can_protoplexer_config_t cfg{};
    cfg.own_address = to;
    cfg.max_chunk_length = 8;
    cfg.max_channels = 8;
    cfg.monitoring = false;
    REQUIRE(uni_can_protoplexer_init(&ctx, &cfg));

    uni_can_protoplexer_msg_t *rx = nullptr;
    for (std::size_t i = 0; i < chunks_count; i++) {
        const auto res = uni_can_protoplexer_add_chunk(&ctx, &chunks[i], &rx);
        REQUIRE(res == UNI_CAN_PROTOPLEXER_OK);
    }

    REQUIRE(rx != nullptr);
    REQUIRE(rx->message_id == msg_id);
    REQUIRE(rx->address_from == from);
    REQUIRE(rx->address_to == to);
    REQUIRE(rx->priority_inverted == prio);
    REQUIRE(rx->length == payload.size());

    for (std::size_t i = 0; i < payload.size(); i++) {
        REQUIRE(rx->data[i] == payload[i]);
    }

    uni_can_protoplexer_msg_free(rx);
    uni_can_protoplexer_deinit(&ctx);
    uni_can_protoplexer_free_chunks(chunks);
    uni_can_protoplexer_msg_free(msg);
}
