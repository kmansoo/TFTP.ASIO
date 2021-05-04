/* TFTPTransaction.c 
   Copyright (c) 2013 James Northway

   Modified by Mansoo Kim(mansoo.kim@icloud.com), 2016.8.26
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <chrono>

//  for ntohs and htons
//  If you can implement another methods, the following header will be not needed.
#include <boost/asio.hpp>

#include "TFTPTransaction.h"
#include "TFTPFile.h"

TFTPTransaction::TFTPTransaction(TFTPTransport* transport, const std::string& root_path) : transport_(transport), root_path_(root_path) {

    init_variables();
}

TFTPTransaction::~TFTPTransaction() {
    packet_free();
}

void TFTPTransaction::init_variables() {
    packet_in_length_ = 0;

    packet_out_ = NULL;
    packet_out_length_ = 0;

    transaction_.timeout_count = 0;
    transaction_.blocknum = 0;
    transaction_.final_packet = 0;
    transaction_.file_open = 0;
    transaction_.complete = 0;
    transaction_.rebound_socket = 0;
    transaction_.ecode = TFTP_ECODE_NONE;

    transaction_.filepos = 0;

    strcpy(transaction_.mode, TFTP_MODE_NETASCII);
}

// STATE FUNCTIONS
void TFTPTransaction::finite_state_machine_client(int *state, int *operation) {
    switch (*state) {
    case TFTP_STATE_SEND:

        switch (*operation) {
        case TFTP_OPERATION_DONE:
            *state = TFTP_STATE_WAIT;
            break;
        case TFTP_OPERATION_FAILED:
            *state = TFTP_STATE_FINISHED;
            break;
        case TFTP_OPERATION_ABANDONED:
            *state = TFTP_STATE_FINISHED;
            break;
        }
        break;

    case TFTP_STATE_WAIT:
        switch (*operation) {
        case TFTP_OPERATION_DONE:
            *state = TFTP_STATE_RECEIVE;
            break;
        case TFTP_OPERATION_FAILED:
            *state = TFTP_STATE_SEND;
            break;
        case TFTP_OPERATION_ABANDONED:
            *state = TFTP_STATE_FINISHED;
            break;
        }
        break;

    case TFTP_STATE_RECEIVE:
        switch (*operation) {
        case TFTP_OPERATION_DONE:
            *state = TFTP_STATE_SEND;
            break;
        case TFTP_OPERATION_ABANDONED:
            *state = TFTP_STATE_FINISHED;
            break;
        }
        break;
    }
}

void TFTPTransaction::finite_state_machine_server(int *state, int *operation) {
    switch (*state) {
    case TFTP_STATE_STANDBY:

        switch (*operation) {
        case TFTP_OPERATION_DONE:
            *state = TFTP_STATE_RECEIVE;
            break;
        }

        break;
    case TFTP_STATE_RECEIVE:

        switch (*operation) {
        case TFTP_OPERATION_DONE:
            *state = TFTP_STATE_SEND;
            break;
        case TFTP_OPERATION_ABANDONED:
            *state = TFTP_STATE_RESET;
            break;
        }

        break;
    case TFTP_STATE_WAIT:

        switch (*operation) {
        case TFTP_OPERATION_DONE:
            *state = TFTP_STATE_RECEIVE;
            break;
        case TFTP_OPERATION_FAILED:
            *state = TFTP_STATE_SEND;
            break;
        case TFTP_OPERATION_ABANDONED:
            *state = TFTP_STATE_RESET;
            break;
        }

        break;
    case TFTP_STATE_SEND:

        switch (*operation) {
        case TFTP_OPERATION_DONE:
            *state = TFTP_STATE_WAIT;
            break;
        case TFTP_OPERATION_ABANDONED:
            *state = TFTP_STATE_RESET;
            break;
        }

        break;
    case TFTP_STATE_RESET:

        switch (*operation) {
        case TFTP_OPERATION_DONE:
            *state = TFTP_STATE_STANDBY;
            break;
        }

        break;
    }
}

void TFTPTransaction::packet_extract_opcode(tftp_packet_t *packet_, const tftp_packetbuffer_t *pbuf) {
    memmove(&packet_->opcode,pbuf,sizeof(opcode_t));
    packet_->opcode = ntohs(packet_->opcode);
}

void TFTPTransaction::packet_parse_rq(tftp_packet_t *packet_, const tftp_packetbuffer_t *pbuf) { 
    strcpy(packet_->filename, pbuf+sizeof(opcode_t));
    strcpy(packet_->mode, pbuf+sizeof(opcode_t)+strlen(packet_->filename)+1);
}

void TFTPTransaction::packet_parse_data(tftp_packet_t *packet_, const tftp_packetbuffer_t *pbuf, 
        int * data_length) {       
    memcpy(&packet_->blocknum,pbuf+sizeof(opcode_t),sizeof(tftp_bnum_t));
    packet_->blocknum = ntohs(packet_->blocknum);
    packet_->data_length = *data_length - sizeof(opcode_t) - sizeof(tftp_bnum_t);
    memcpy(packet_->data, pbuf+sizeof(opcode_t)+sizeof(tftp_bnum_t), 
        packet_->data_length);
}

void TFTPTransaction::packet_parse_ack(tftp_packet_t *packet_, const tftp_packetbuffer_t *pbuf) {
    memcpy(&packet_->blocknum,pbuf+sizeof(opcode_t),sizeof(tftp_bnum_t));
    packet_->blocknum = ntohs(packet_->blocknum);
}

void TFTPTransaction::packet_parse_error(tftp_packet_t *packet_, const tftp_packetbuffer_t *pbuf, 
        int * data_length) {      
    memcpy(&packet_->ecode,pbuf+sizeof(opcode_t),sizeof(ecode_t));
    packet_->ecode = ntohs(packet_->ecode);
    packet_->estring_length = 
        *data_length - sizeof(opcode_t) - sizeof(ecode_t);
    memcpy(packet_->estring, pbuf+sizeof(opcode_t)+sizeof(ecode_t),
        packet_->estring_length);
}

tftp_packetbuffer_t *TFTPTransaction::append_to_packet(const void *data, const int data_size) {
    packet_out_ = (tftp_packetbuffer_t *)realloc(packet_out_,
        sizeof(tftp_packetbuffer_t)*packet_out_length_ + data_size);
    memmove(packet_out_ + packet_out_length_, data, data_size);
    packet_out_length_ += data_size;
    return packet_out_;
}

void TFTPTransaction::packet_form_rrq(char * filename) {
    opcode_t opcode_out = htons(TFTP_OPCODE_RRQ);
    append_to_packet(&opcode_out, sizeof(opcode_t));
    append_to_packet(filename, strlen(filename)+1);
    append_to_packet(transaction_.mode, strlen(transaction_.mode)+1);
}

void TFTPTransaction::packet_form_wrq(char * outfilename) {
    opcode_t opcode_out = htons(TFTP_OPCODE_WRQ);
    append_to_packet(&opcode_out, sizeof(opcode_t));
    append_to_packet(outfilename, strlen(outfilename)+1);
    append_to_packet(transaction_.mode, strlen(transaction_.mode)+1);
}

void TFTPTransaction::packet_form_data() {
    opcode_t opcode_out = htons(TFTP_OPCODE_DATA);
    tftp_bnum_t new_blocknum = htons(transaction_.blocknum);
    append_to_packet(&opcode_out, sizeof(opcode_t));
    append_to_packet(&new_blocknum, sizeof(tftp_bnum_t));
    append_to_packet(&transaction_.filebuffer, transaction_.filebuffer_length);
    //  printf("# Transferred block: %i\n", transaction_.blocknum);
}

void TFTPTransaction::packet_form_error() {
    opcode_t opcode_out = htons(TFTP_OPCODE_ERROR);
    ecode_t new_ecode = htons(transaction_.ecode);
    append_to_packet(&opcode_out, sizeof(opcode_t));
    append_to_packet(&new_ecode, sizeof(ecode_t));
    append_to_packet(transaction_.estring, strlen(transaction_.estring)+1); 
}

void TFTPTransaction::packet_form_ack() {
    opcode_t opcode_out = htons(TFTP_OPCODE_ACK);
    tftp_bnum_t new_blocknum = htons(transaction_.blocknum);

    //  printf("# Received block: %i\n", transaction_.blocknum);

    append_to_packet(&opcode_out, sizeof(opcode_t));
    append_to_packet(&new_blocknum, sizeof(tftp_bnum_t));
}

int TFTPTransaction::packet_parse(tftp_packet_t *packet_, const tftp_packetbuffer_t *pbuf, 
        int *packet_in_length) {  
    packet_extract_opcode(packet_,pbuf);
    
    if (TFTP_IS_RRQ(packet_->opcode) || TFTP_IS_WRQ(packet_->opcode)) {
        packet_parse_rq(packet_, pbuf);
    }else if (TFTP_IS_ACK(packet_->opcode)) {
        packet_parse_ack(packet_, pbuf);
    }else if (TFTP_IS_DATA(packet_->opcode)) {
        packet_parse_data(packet_, pbuf, packet_in_length);
    }else if (TFTP_IS_ERROR(packet_->opcode)) {
        packet_parse_error(packet_, pbuf, packet_in_length);
    } else {
        return -1;
    }
    
    return 0;
}

void TFTPTransaction::packet_free() {
    if (packet_out_length_ > 0) {
        free(packet_out_);
        packet_out_length_ = 0;
        packet_out_ = NULL;
    }
}

int TFTPTransaction::packet_receive_rrq() {
    //  printf("# Read request for %s\n",packet_.filename);
    packet_free();
        
    if (transaction_.file_open == 1) {
        file_->close();
        file_ = nullptr;
    }

    file_ = std::make_shared<TFTPFile>();

    //  make filename using root_path
    std::string filename;

    if (root_path_.length() > 0) {
        filename = root_path_;
        filename += '/';
    }

    filename += packet_.filename;
    
    if ((file_->open_read((char*)filename.c_str()))== -1) {
        strcpy(transaction_.estring, TFTP_ESTRING_1);
        transaction_.ecode = TFTP_ECODE_1;
        packet_form_error();

        file_ = nullptr;
    } else {
        transaction_.file_open = 1;
        transaction_.blocknum = 1;
        transaction_.filepos = ((transaction_.blocknum * TFTP_BLIMIT) - TFTP_BLIMIT);
        transaction_.filebuffer_length = file_->read_buffer_from_pos(&transaction_);
        packet_form_data();

        if (transport_) {
            int max_block_size = (file_->get_length() / TFTP_BLIMIT) + 1;
            transport_->on_tftp_start_transaction(max_block_size);

            if (transport_)
                transport_->on_tftp_progress_transaction(transaction_.blocknum);
        }
    }
    
    return TFTP_OPERATION_DONE;
}

int TFTPTransaction::packet_receive_wrq() {
    //  printf("# Write request for %s\n",packet_.filename);
    packet_free();
    
    if (transaction_.file_open == 0) { 
        file_ = std::make_shared<TFTPFile>();

        //  make filename using root_path
        std::string filename;

        //  make filename using root_path
        if (root_path_.length() > 0) {
            filename = root_path_;
            filename += '/';
        }

        filename += packet_.filename;

        if ((file_->open_write((char*)filename.c_str())) == 0) {
            transaction_.file_open = 1;
            
        } else {
            transaction_.file_open = 0;
            file_ = nullptr;
        }
    }
    
    if (transaction_.file_open == 0) {
        strcpy(transaction_.estring, TFTP_ESTRING_1);
        transaction_.ecode = TFTP_ECODE_1;
        packet_form_error();       
    } else {

        if (transport_)
            transport_->on_tftp_start_transaction(0);

        packet_form_ack();
        transaction_.blocknum++;
    }
    
    return TFTP_OPERATION_DONE;
}

int TFTPTransaction::packet_receive_data() {
    if (packet_.blocknum == transaction_.blocknum) {  
        if (file_ && file_->append_from_buffer(&packet_, &transaction_) == -1) {
            strcpy(transaction_.estring, TFTP_ESTRING_2);
            transaction_.ecode = TFTP_ECODE_2;
            packet_free();
            packet_form_error();           
        } else {

            if (transport_)
                transport_->on_tftp_progress_transaction(transaction_.blocknum);

            packet_free();
            packet_form_ack();
            transaction_.blocknum++;
        }
    }
    
    if (file_ && packet_.data_length < 512) {
        file_->close();
        file_ = nullptr;

        transaction_.file_open = 0;
        transaction_.complete = 1;

        if (transport_)
            transport_->on_tftp_completed_transaction();
    }
    
    return TFTP_OPERATION_DONE;
}

int TFTPTransaction::packet_receive_ack() {

    if (packet_.blocknum == transaction_.blocknum) {
        transaction_.blocknum++;
        transaction_.timeout_count = 0;
        packet_free();
        
        //  make filename using root_path
        std::string filename;

        if (root_path_.length() > 0) {
            filename = root_path_;
            filename += '/';
        }

        filename += packet_.filename;

        if (transaction_.file_open == 0 && (file_->open_read((char*)filename.c_str())) == -1) {
            strcpy(transaction_.estring, TFTP_ESTRING_2);
            transaction_.ecode = TFTP_ECODE_2;
            packet_form_error();            
        } else {
            transaction_.filepos = ((transaction_.blocknum * TFTP_BLIMIT) - TFTP_BLIMIT);
            transaction_.filebuffer_length = file_->read_buffer_from_pos(&transaction_);

            if (!transaction_.filebuffer_length) {
                transaction_.complete = 1;

                if (transport_)
                    transport_->on_tftp_completed_transaction();

                return TFTP_OPERATION_ABANDONED;               
            } else {
                packet_form_data();

                if (transport_)
                    transport_->on_tftp_progress_transaction(transaction_.blocknum);
            }
        }
    }
    
    return TFTP_OPERATION_DONE;
}

int TFTPTransaction::packet_receive_error() {
    fprintf(stderr, "# Received error %i: %s\n",packet_.ecode, packet_.estring);
    return TFTP_OPERATION_ABANDONED;
}

int TFTPTransaction::packet_receive_invalid() { 
    transaction_.ecode = TFTP_ECODE_4;
    strcpy(transaction_.estring, TFTP_ESTRING_4);
    packet_free();
    packet_form_error();
    return TFTP_OPERATION_DONE;
}