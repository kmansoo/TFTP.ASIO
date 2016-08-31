/* TFTPTransaction.h 
   Copyright (c) 2013 James Northway

   Modified by Mansoo Kim(mansoo.kim@icloud.com), 2016.8.26
*/

#ifndef TFTP_TRANSATION
#define TFTP_TRANSATION

#include <memory>
#include <thread>

#include "TFTPPacket.h"
#include "TFTPTransport.h"
#include "TFTPFile.h"

//  TFTPTransaction class

class TFTPTransaction {
public:
    TFTPTransaction(TFTPTransport* transport, const std::string& root_path = "");
    ~TFTPTransaction();

protected:
    enum FSM_State {
        TFTP_STATE_STANDBY = 1,
        TFTP_STATE_RECEIVE = 2,
        TFTP_STATE_WAIT = 3,
        TFTP_STATE_SEND = 4,
        TFTP_STATE_RESET = 5,
        TFTP_STATE_FINISHED = 9
    };

    enum FSM_Operation {
        TFTP_OPERATION_DONE = 0,
        TFTP_OPERATION_FAILED = 1,
        TFTP_OPERATION_ABANDONED = 2
    };

protected:
    void init_variables();

    void finite_state_machine_client(int *state, int *operation);
    void finite_state_machine_server(int *state, int *operation);

    /* Packet Parsing */
    int packet_parse(tftp_packet_t *packet, const tftp_packetbuffer_t *pbuf, int *packet_in_length);
    void packet_extract_opcode(tftp_packet_t *packet, const tftp_packetbuffer_t *pbuf);
    void packet_parse_rq(tftp_packet_t *packet, const tftp_packetbuffer_t *pbuf);
    void packet_parse_data(tftp_packet_t *packet, const tftp_packetbuffer_t *pbuf, int *packet_in_length);
    void packet_parse_ack(tftp_packet_t *packet, const tftp_packetbuffer_t *pbuf);
    void packet_parse_error(tftp_packet_t *packet, const tftp_packetbuffer_t *pbuf, int *packet_in_length);

    /* Packet Formation */
    tftp_packetbuffer_t *append_to_packet(const void *data, const int data_size);
    void packet_form_rrq(char * filename);
    void packet_form_wrq(char * filename);
    void packet_form_data();
    void packet_form_error();
    void packet_form_ack();
    void packet_free();

    /* Packet Receipt */
    int packet_receive_rrq();
    int packet_receive_wrq();
    int packet_receive_data();
    int packet_receive_ack();
    int packet_receive_error();
    int packet_receive_invalid();

public:
    int stateOfTFTP_;

    std::shared_ptr<TFTPFile> file_;
    TFTPTransport* transport_;
    std::string root_path_;

    tftp_packetbuffer_t packet_in_buffer_[TFTP_PACKETSIZE];
    int packet_in_length_;
    tftp_packetbuffer_t *packet_out_;
    int packet_out_length_;
    struct tftp_packet packet_;
    struct tftp_transaction transaction_;
};

#endif
