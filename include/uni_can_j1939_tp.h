#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

//
// Includes
//

// stdlib
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

// Uni.CAN
#include "uni_can_message.h"



//
// Typedefs
//

typedef enum {
    /**
     * Request to Send
     */
    UNI_CAN_J1939_TP_CM_TYPE_RTS = 16,

    /**
     * Clear to Send
     */
    UNI_CAN_J1939_TP_CM_TYPE_CTS = 17,

    /**
     * End of Message Acknowledgment
     */
    UNI_CAN_J1939_TP_CM_TYPE_EOM = 19,

    /**
     * Broadcast Announce Message
     */
    UNI_CAN_J1939_TP_CM_TYPE_BAM = 32,

    /**
     * Connection Abort
     */
    UNI_CAN_J1939_TP_CM_TYPE_ABORT = 255,
} uni_can_j1939_tp_cm_type_e;


typedef enum {
    UNI_CAN_J1939_TP_SESSION_STATE_UNKNOWN = 0,

    UNI_CAN_J1939_TP_SESSION_STATE_Other_Finished = 10,
    UNI_CAN_J1939_TP_SESSION_STATE_Other_Abort = 11,
    UNI_CAN_J1939_TP_SESSION_STATE_Other_Drop = 12,
    UNI_CAN_J1939_TP_SESSION_STATE_Other_Timeout = 13,

    UNI_CAN_J1939_TP_SESSION_STATE_Incoming_Burst_Init = 30,
    UNI_CAN_J1939_TP_SESSION_STATE_Incoming_Burst_Data = 31,
    UNI_CAN_J1939_TP_SESSION_STATE_Incoming_Burst_WrongOrder = 32,
    UNI_CAN_J1939_TP_SESSION_STATE_Incoming_Burst_BurstEnd = 33,
    UNI_CAN_J1939_TP_SESSION_STATE_Incoming_Burst_MessageEnd = 34,

    UNI_CAN_J1939_TP_SESSION_STATE_IN_BROADCAST_DATA = 40,

    UNI_CAN_J1939_TP_SESSION_STATE_Outgoing_Burst_Init = 50,
    UNI_CAN_J1939_TP_SESSION_STATE_Outgoing_Burst_Waiting = 51,
    UNI_CAN_J1939_TP_SESSION_STATE_Outgoing_Burst_Data = 52,

    UNI_CAN_J1939_TP_SESSION_STATE_OUT_BROADCAST_INIT = 60,
    UNI_CAN_J1939_TP_SESSION_STATE_OUT_BROADCAST_DATA = 61,
} uni_can_j1939_tp_session_state_e;

#pragma pack(push, 1)
/**
 * J1939/21 5.10.3.5, Broadcast Announce Message (TP.CM_BAM): Global Destination
 *
 * The TP.CM_BAM is used to inform all the nodes of the network that a large message is about to be broadcast. It defines
 * the parameter group and the number of bytes to be sent. After TP.CM_BAM is sent, the Data Transfer Messages are
 * sent and they contain the packetized broadcast data.
 *
 * TP.CM_BAM is only transmitted by the originator.
 */
typedef struct {
    /**
     * [0] Control byte.
     * Must be 32 (UNI_CAN_J1939_TP_CM_TYPE_BAM)
     */
    uint8_t control;

    /**
     * [1-2] Total message size, number of bytes
     */
    uint16_t message_size;

    /**
     * [3] Total number of packets
     */
    uint8_t number_of_packets;

    /**
     * [4] Reserved for assignment by SAE, this byte should be filled with FF16
     */
    uint8_t reserved;

    /**
     * [5-7] Parameter Group Number of the packeted message
     */
    uint8_t pgn[3];
} uni_can_j1939_tp_cm_bam_t;

static_assert(sizeof(uni_can_j1939_tp_cm_bam_t) == 8, "");
#pragma pack(pop)


#pragma pack(push, 1)
/**
 * J1939/21 5.10.4, Data Transfer Message (TP.DT): Destination Specific
 *
 * The TP.DT message is used to communicate the data associated with a Parameter Group.
 *
 * The TP.DT message is an individual packet of a multipacket message transfer.
 * For example if a large message had to be divided into 5 packets in order to be communicated, then there would be 5 TP.DT messages.
 */
typedef struct {
    /**
    * [0] Sequence number
    */
    uint8_t seq_num;

    /**
     * [1-7] Packetized Data.
     *
     * Note the last packet of a multipacket Parameter Group may require less than 8 data bytes.
     * The extra bytes should be filled with FF16.
     */
    uint8_t data[7];
} uni_can_j1939_tp_dt_t;

static_assert(sizeof(uni_can_j1939_tp_dt_t) == 8, "");
#pragma pack(pop)


typedef struct {
    /**
     * Total message size, number of bytes
     */
    uint16_t message_size;

    /**
     * For incoming -- destination address
     * For outgoing -- source address
     */
    uint8_t address_local;

    /**
     * For incoming -- source address
     * For outgoing -- destination address
     */
    uint8_t address_remote;

    /**
     * Total number of packets
     */
    uint8_t number_of_packets;

    /**
     * Maximum number of packets that can be sent in response to one CTS.
     */
    uint8_t burst_count;
} uni_can_j1939_tp_session_settings_t;

typedef struct {
    /**
     * Session settings
     */
    uni_can_j1939_tp_session_settings_t settings;

    /**
     * Session state
     */
    uni_can_j1939_tp_session_state_e state;

    /**
     * Incoming/Outgoing message
     */
    uni_can_message_t* message;

    size_t chunks_trx;

    time_t time_rx;

    time_t time_tx;
} uni_can_j1939_tp_session_t;


#if defined(__cplusplus)
 }
#endif
