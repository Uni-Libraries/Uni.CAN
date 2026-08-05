// stdlib
#include <array>
#include <cstdint>

// catch2
#include <catch2/catch_test_macros.hpp>

// uni.can
#include "uni_can_protocol_imp.h"

namespace {

uni_can_imp_ctx_t make_context(std::uint8_t address, std::uint32_t timeout_ms = 1000,
                               uni_can_imp_message_validator_t validator = nullptr) {
    uni_can_imp_ctx_t ctx{};
    uni_can_imp_config_t config{};
    config.own_address = address;
    config.max_sessions = 8;
    config.max_message_size = 200;
    config.session_timeout_ms = timeout_ms;
    config.message_validator = validator;
    REQUIRE(uni_can_imp_init(&ctx, &config));
    return ctx;
}

bool reject_message(const uni_can_imp_msg_t *, void *) {
    return false;
}

} // namespace

TEST_CASE("imp_canid_mapping", "[imp]") {
    const std::uint32_t can_id = uni_can_imp_canid_create(1, 2, 3, UNI_CAN_IMP_CMD_DATA);

    REQUIRE(can_id == 0x02099000U);
    REQUIRE(uni_can_imp_canid_get_from(can_id) == 1);
    REQUIRE(uni_can_imp_canid_get_to(can_id) == 2);
    REQUIRE(uni_can_imp_canid_get_session_id(can_id) == 3);
    REQUIRE(uni_can_imp_canid_get_command(can_id) == UNI_CAN_IMP_CMD_DATA);
}

TEST_CASE("imp_transfer_without_ack", "[imp]") {
    uni_can_imp_ctx_t sender = make_context(1);
    uni_can_imp_ctx_t receiver = make_context(2);

    std::array<std::uint8_t, 18> payload{};
    for (std::size_t i = 0; i < payload.size(); i++) {
        payload[i] = static_cast<std::uint8_t>(i + 1);
    }

    uni_can_imp_msg_t *message = uni_can_imp_msg_create(UNI_CAN_IMP_MSG_MODE_WITHOUT_ACK, 1, 2,
                                                         payload.data(), payload.size());
    REQUIRE(message != nullptr);

    uni_can_imp_output_t tx{};
    uni_can_imp_output_t rx{};
    REQUIRE(uni_can_imp_start_send(&sender, message, 0, &tx) == UNI_CAN_IMP_OK);
    REQUIRE(tx.frame_ready);
    REQUIRE(uni_can_imp_canid_get_command(tx.frame.id) == UNI_CAN_IMP_CMD_START_WA);
    REQUIRE(uni_can_imp_add_frame(&receiver, &tx.frame, 1, &rx) == UNI_CAN_IMP_OK);
    REQUIRE_FALSE(rx.frame_ready);

    uni_can_imp_msg_t *received = nullptr;
    std::size_t frame_count = 1;
    for (std::uint32_t now = 2; !received; now++) {
        REQUIRE(uni_can_imp_process(&sender, now, &tx) == UNI_CAN_IMP_OK);
        REQUIRE(tx.frame_ready);
        frame_count++;

        REQUIRE(uni_can_imp_add_frame(&receiver, &tx.frame, now, &rx) == UNI_CAN_IMP_OK);
        if (rx.received_message) {
            received = rx.received_message;
        }
    }

    REQUIRE(frame_count == 5);
    REQUIRE(tx.event == UNI_CAN_IMP_EVENT_TX_FINISHED);
    REQUIRE(received->mode == UNI_CAN_IMP_MSG_MODE_WITHOUT_ACK);
    REQUIRE(received->address_from == 1);
    REQUIRE(received->address_to == 2);
    REQUIRE(received->length == payload.size());
    for (std::size_t i = 0; i < payload.size(); i++) {
        REQUIRE(received->data[i] == payload[i]);
    }

    uni_can_imp_msg_free(received);
    uni_can_imp_msg_free(message);
    uni_can_imp_deinit(&receiver);
    uni_can_imp_deinit(&sender);
}

TEST_CASE("imp_transfer_with_ack", "[imp]") {
    uni_can_imp_ctx_t sender = make_context(7);
    uni_can_imp_ctx_t receiver = make_context(3);

    std::array<std::uint8_t, 17> payload{};
    for (std::size_t i = 0; i < payload.size(); i++) {
        payload[i] = static_cast<std::uint8_t>(0xA0U + i);
    }

    uni_can_imp_msg_t *message =
        uni_can_imp_msg_create(UNI_CAN_IMP_MSG_MODE_WITH_ACK, 7, 3, payload.data(), payload.size());
    REQUIRE(message != nullptr);

    uni_can_imp_output_t sender_output{};
    uni_can_imp_output_t receiver_output{};
    REQUIRE(uni_can_imp_start_send(&sender, message, 0, &sender_output) == UNI_CAN_IMP_OK);
    REQUIRE(uni_can_imp_canid_get_session_id(sender_output.frame.id) == 1);
    REQUIRE(uni_can_imp_canid_get_command(sender_output.frame.id) == UNI_CAN_IMP_CMD_START);

    REQUIRE(uni_can_imp_add_frame(&receiver, &sender_output.frame, 1, &receiver_output) == UNI_CAN_IMP_OK);
    REQUIRE(receiver_output.frame_ready);
    REQUIRE(uni_can_imp_canid_get_command(receiver_output.frame.id) == UNI_CAN_IMP_CMD_ACK);

    uni_can_imp_msg_t *received = nullptr;
    for (std::uint32_t now = 2; sender_output.event != UNI_CAN_IMP_EVENT_TX_FINISHED; now += 2) {
        REQUIRE(receiver_output.frame_ready);
        REQUIRE(uni_can_imp_add_frame(&sender, &receiver_output.frame, now, &sender_output) == UNI_CAN_IMP_OK);
        if (sender_output.event == UNI_CAN_IMP_EVENT_TX_FINISHED) {
            break;
        }

        REQUIRE(sender_output.frame_ready);
        REQUIRE(uni_can_imp_add_frame(&receiver, &sender_output.frame, now + 1, &receiver_output) == UNI_CAN_IMP_OK);
        if (receiver_output.received_message) {
            received = receiver_output.received_message;
        }
    }

    REQUIRE(received != nullptr);
    REQUIRE(received->session_id == 1);
    REQUIRE(received->mode == UNI_CAN_IMP_MSG_MODE_WITH_ACK);
    REQUIRE(received->length == payload.size());
    for (std::size_t i = 0; i < payload.size(); i++) {
        REQUIRE(received->data[i] == payload[i]);
    }

    uni_can_imp_msg_free(received);
    uni_can_imp_msg_free(message);
    uni_can_imp_deinit(&receiver);
    uni_can_imp_deinit(&sender);
}

TEST_CASE("imp_validator_rejects_message", "[imp]") {
    uni_can_imp_ctx_t sender = make_context(1);
    uni_can_imp_ctx_t receiver = make_context(2, 1000, reject_message);
    uni_can_imp_msg_t *message = uni_can_imp_msg_create(UNI_CAN_IMP_MSG_MODE_WITH_ACK, 1, 2, nullptr, 0);
    REQUIRE(message != nullptr);

    uni_can_imp_output_t tx{};
    uni_can_imp_output_t rx{};
    REQUIRE(uni_can_imp_start_send(&sender, message, 0, &tx) == UNI_CAN_IMP_OK);
    REQUIRE(uni_can_imp_add_frame(&receiver, &tx.frame, 1, &rx) == UNI_CAN_IMP_OK);
    REQUIRE(uni_can_imp_add_frame(&sender, &rx.frame, 2, &tx) == UNI_CAN_IMP_OK);
    REQUIRE(uni_can_imp_canid_get_command(tx.frame.id) == UNI_CAN_IMP_CMD_END);

    REQUIRE(uni_can_imp_add_frame(&receiver, &tx.frame, 3, &rx) == UNI_CAN_IMP_MESSAGE_REJECTED);
    REQUIRE(rx.event == UNI_CAN_IMP_EVENT_RX_ABORTED);
    REQUIRE(rx.frame_ready);
    REQUIRE(uni_can_imp_canid_get_command(rx.frame.id) == UNI_CAN_IMP_CMD_NACK);

    REQUIRE(uni_can_imp_add_frame(&sender, &rx.frame, 4, &tx) == UNI_CAN_IMP_MESSAGE_REJECTED);
    REQUIRE(tx.event == UNI_CAN_IMP_EVENT_TX_ABORTED);

    uni_can_imp_msg_free(message);
    uni_can_imp_deinit(&receiver);
    uni_can_imp_deinit(&sender);
}

TEST_CASE("imp_acked_transfer_times_out", "[imp]") {
    uni_can_imp_ctx_t sender = make_context(1, 10);
    uni_can_imp_msg_t *message = uni_can_imp_msg_create(UNI_CAN_IMP_MSG_MODE_WITH_ACK, 1, 2, nullptr, 0);
    REQUIRE(message != nullptr);

    uni_can_imp_output_t output{};
    REQUIRE(uni_can_imp_start_send(&sender, message, 0, &output) == UNI_CAN_IMP_OK);
    REQUIRE(uni_can_imp_process(&sender, 10, &output) == UNI_CAN_IMP_OK);
    REQUIRE(output.event == UNI_CAN_IMP_EVENT_NONE);
    REQUIRE(uni_can_imp_process(&sender, 11, &output) == UNI_CAN_IMP_OK);
    REQUIRE(output.event == UNI_CAN_IMP_EVENT_TX_TIMEOUT);
    REQUIRE(output.peer_address == 2);

    uni_can_imp_msg_free(message);
    uni_can_imp_deinit(&sender);
}
