//
// Includes
//

// stdlib
#include <cmath>

// catch2
#include <catch2/catch_test_macros.hpp>

// Uni.CAN
#include "uni_can.h"



//
// Tests
//

TEST_CASE("j1939_sna_parse", "j1939") {
    //signal
    std::array<uni_can_j1939_signal_t*,2> signals{};
    signals[0] = new uni_can_j1939_signal_t();
    signals[0]->id = 1;
    signals[0]->scale = 1.0;
    signals[0]->type = UNI_CAN_J1939_SIGNAL_SLOT;
    signals[0]->length = 8;
    signals[0]->val_min = 0;
    signals[0]->val_max = 250;

    //msg description
    uni_can_j1939_msg_desc_t desc{};
    desc.pgn_number = 1;
    desc.signal = signals.data();
    desc.period = 0;

    //msg
    auto* msg = uni_can_message_create();
    msg->id=uni_can_j1939_pgn_create(6, 1,0,0);
    msg->flags =UNI_CAN_MSG_FLAG_EXT_ID;
    msg->len = 1;
    msg->data.u8[0]=0xFF;

    //parse
    uni_can_j1939_signal_value_t val{};
    REQUIRE(uni_can_j1939_msg_signal_get(msg, &desc, 1, &val));
    REQUIRE(std::isnan(val.slot));

    // cleanup
    delete signals[0];
    uni_can_message_free(msg);
}