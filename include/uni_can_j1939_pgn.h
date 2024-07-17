#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

//
// Includes
//

// stdlib
#include <stdbool.h>
#include <stdint.h>



//
// Functions
//

uint8_t uni_can_j1939_pgn_get_priority(uint32_t can_id);

uint8_t uni_can_j1939_pgn_get_datapage(uint32_t can_id);

uint8_t uni_can_j1939_pgn_get_pduformat(uint32_t can_id);

uint8_t uni_can_j1939_pgn_get_pduspecific(uint32_t can_id);

uint8_t uni_can_j1939_pgn_get_sourceaddr(uint32_t can_id);

uint32_t uni_can_j1939_pgn_get_pgnnumber_mask(uint32_t can_id);

uint32_t uni_can_j1939_pgn_get_pgnnumber(uint32_t can_id);

bool uni_can_j1939_pgn_contains_dstaddr(uint32_t can_id);

uint32_t uni_can_j1939_pgn_create_3(uint32_t pgn_number);

uint32_t uni_can_j1939_pgn_create_2(uint32_t pgn_number, uint32_t pdu_specific);

uint32_t uni_can_j1939_pgn_create(uint8_t priority, uint32_t pgn_number, uint8_t pdu_specific, uint8_t source_addr);

uint32_t uni_can_j1939_pgn_create_from_array(const uint8_t *data);

void uni_can_j1939_pgn_create_to_array(uint32_t pgn, uint8_t *data);

uint32_t uni_can_j1939_pgn_replace_srcaddr(uint32_t can_id, uint8_t src_addr);

uint32_t uni_can_j1939_pgn_replace_dstaddr(uint32_t can_id, uint8_t dst_addr);


#if defined(__cplusplus)
}
#endif
