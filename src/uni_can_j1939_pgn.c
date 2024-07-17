//
// Includes
//

// uni.can
#include "uni_can_j1939_const.h"
#include "uni_can_j1939_pgn.h"


//
// Defines
//

#define EXT_PDU_FORMAT 240U


//
// Functions
//

uint8_t uni_can_j1939_pgn_get_priority(uint32_t can_id) {
    return (can_id & UNI_CAN_J1939_MASK_PRIORITY) >> UNI_CAN_J1939_POS_PRIORITY;
}


uint8_t uni_can_j1939_pgn_get_datapage(uint32_t can_id) {
    return (can_id & UNI_CAN_J1939_MASK_DATAPAGE) >> UNI_CAN_J1939_POS_DATAPAGE;
}


uint8_t uni_can_j1939_pgn_get_pduformat(uint32_t can_id) {
    return (can_id & UNI_CAN_J1939_MASK_PDU_FORMAT) >> UNI_CAN_J1939_POS_PDU_FORMAT;
}


uint8_t uni_can_j1939_pgn_get_pduspecific(uint32_t can_id) {
    return (can_id & UNI_CAN_J1939_MASK_PDU_SPECIFIC) >> UNI_CAN_J1939_POS_PDU_SPECIFIC;
}


uint8_t uni_can_j1939_pgn_get_sourceaddr(uint32_t can_id) {
    return (can_id & UNI_CAN_J1939_MASK_ADDR_SRC) >> UNI_CAN_J1939_POS_ADDR_SRC;
}


uint32_t uni_can_j1939_pgn_get_pgnnumber_mask(uint32_t can_id) {
    uint32_t result = 0U;
    if (uni_can_j1939_pgn_contains_dstaddr(can_id)) {
        result = UNI_CAN_J1939_MASK_DATAPAGE | UNI_CAN_J1939_MASK_PDU_FORMAT;
    } else {
        result = UNI_CAN_J1939_MASK_DATAPAGE | UNI_CAN_J1939_MASK_PDU_FORMAT | UNI_CAN_J1939_MASK_PDU_SPECIFIC;
    }
    return result;
}


uint32_t uni_can_j1939_pgn_get_pgnnumber(uint32_t can_id) {
    return (can_id & uni_can_j1939_pgn_get_pgnnumber_mask(can_id)) >> UNI_CAN_J1939_POS_PGN_NUMBER;
}


bool uni_can_j1939_pgn_contains_dstaddr(uint32_t can_id) {
    return uni_can_j1939_pgn_get_pduformat(can_id) < EXT_PDU_FORMAT;
}


uint32_t uni_can_j1939_pgn_create_3(uint32_t pgn_number) {
    return (pgn_number << UNI_CAN_J1939_POS_PGN_NUMBER) & uni_can_j1939_pgn_get_pgnnumber_mask(
               pgn_number << UNI_CAN_J1939_POS_PGN_NUMBER);
}


uint32_t uni_can_j1939_pgn_create_2(uint32_t pgn_number, uint32_t pdu_specific) {
    uint32_t result = uni_can_j1939_pgn_create_3(pgn_number);
    if (uni_can_j1939_pgn_contains_dstaddr(result)) {
        result |= (pdu_specific & 0xFF) << UNI_CAN_J1939_POS_PDU_SPECIFIC;
    }

    return result;
}


uint32_t uni_can_j1939_pgn_create(uint8_t priority, uint32_t pgn_number, uint8_t pdu_specific, uint8_t source_addr) {
    uint32_t result = 0U;

    // priority
    result |= (priority << UNI_CAN_J1939_POS_PRIORITY) & UNI_CAN_J1939_MASK_PRIORITY;

    // pgn number and pdu specific
    result |= uni_can_j1939_pgn_create_2(pgn_number, pdu_specific);

    // source address
    result |= (source_addr << UNI_CAN_J1939_POS_ADDR_SRC) & UNI_CAN_J1939_MASK_ADDR_SRC;

    return result;
}


uint32_t uni_can_j1939_pgn_create_from_array(const uint8_t *data) {
    uint32_t pgn = 0;
    if (data) {
        pgn |= data[0] << 0;
        pgn |= data[1] << 8;
        pgn |= data[2] << 16;
    }
    return pgn;
}


void uni_can_j1939_pgn_create_to_array(uint32_t pgn, uint8_t *data) {
    if (data) {
        data[0] = (pgn >> 0) & 0xFF; //-V557
        data[1] = (pgn >> 8) & 0xFF; //-V557
        data[2] = (pgn >> 16) & 0x03; //-V557
    }
}


uint32_t uni_can_j1939_pgn_replace_addr(uint32_t can_id, uint8_t src_addr) {
    return (can_id & (~UNI_CAN_J1939_MASK_ADDR_SRC)) | (src_addr & UNI_CAN_J1939_MASK_ADDR_SRC);
}


uint32_t uni_can_j1939_pgn_replace_dstaddr(uint32_t can_id, uint8_t dst_addr) {
    uint32_t result = can_id;
    if (uni_can_j1939_pgn_contains_dstaddr(result)) {
        result = (result & (~UNI_CAN_J1939_MASK_PDU_SPECIFIC)) | (
                     (dst_addr << UNI_CAN_J1939_POS_PDU_SPECIFIC) & UNI_CAN_J1939_MASK_PDU_SPECIFIC);
    }
    return result;
}
