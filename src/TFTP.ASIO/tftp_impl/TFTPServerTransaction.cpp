/* TFTPTransaction.c 
   Copyright (c) 2013 James Northway

   Modified by Mansoo Kim(mansoo.kim@icloud.com), 2016.8.26
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <chrono>
#include <iostream>

#include "TFTPServerTransaction.h"
#include "TFTPFile.h"

TFTPServerTransaction::TFTPServerTransaction(TFTPTransport* transport, const std::string& root_path) : 
    TFTPTransaction(transport, root_path), is_stop_(false) {

    stateOfTFTP_ = TFTP_STATE_STANDBY;

    thread_ = std::thread([&] {
        int operation = 0;

        while (is_stop_ == false) {

            // We're going to wait for 1 millisecond.
            if (stateOfTFTP_ == TFTP_STATE_WAIT || stateOfTFTP_ == TFTP_STATE_STANDBY) {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait_for(lk, std::chrono::milliseconds(1));
            }

            switch (stateOfTFTP_) {
            case TFTP_STATE_STANDBY:
                operation = state_standby();
                break;

            case TFTP_STATE_RECEIVE:
                operation = state_receive();
                break;

            case TFTP_STATE_WAIT:
                operation = state_wait();
                break;

            case TFTP_STATE_SEND:
                operation = state_send();
                break;

            case TFTP_STATE_RESET:
                operation = state_reset();
                break;
            }

            finite_state_machine_server(&stateOfTFTP_, &operation);
        }
    });
}

TFTPServerTransaction::~TFTPServerTransaction() {
    stop();
    cv_.notify_one();

    thread_.join();
}


bool TFTPServerTransaction::stop() {
    is_stop_ = true;

    return true;
}

void TFTPServerTransaction::data_event() {
    cv_.notify_one();
}

int TFTPServerTransaction::state_standby() {
    int operation = TFTP_OPERATION_FAILED;

    if (transport_->tftp_has_received_data()) {
        operation = TFTP_OPERATION_DONE;
        packet_in_length_ = transport_->tftp_get_received_data(packet_in_buffer_, TFTP_PACKETSIZE);
    }

    return operation;
}

int TFTPServerTransaction::state_receive() {

    int operation = -1;

    packet_parse(&packet_, packet_in_buffer_, &packet_in_length_);

    if (TFTP_IS_RRQ(packet_.opcode)) {
        operation = packet_receive_rrq();
    }
    else if (TFTP_IS_WRQ(packet_.opcode)) {
        operation = packet_receive_wrq();
    }
    else if (TFTP_IS_ACK(packet_.opcode)) {
        operation = packet_receive_ack();
    }
    else if (TFTP_IS_DATA(packet_.opcode)) {
        operation = packet_receive_data();
    }
    else if (TFTP_IS_ERROR(packet_.opcode)) {
        operation = packet_receive_error();
    }
    else {
        operation = packet_receive_invalid();
    }

    return operation;
}

int TFTPServerTransaction::state_wait() {
    int operation = -1;

    transaction_.timed_out = 0;

    if (transport_) {
        if (transport_->tftp_has_received_data()) {
            packet_in_length_ = transport_->tftp_get_received_data(packet_in_buffer_, TFTP_PACKETSIZE);

            transaction_.timeout_count = 0;

            operation = TFTP_OPERATION_DONE;
        }
        else {
            if (transport_->tftp_is_connect() == false)
                operation = TFTP_OPERATION_FAILED;
        }
    }

    if (operation == -1) {
        transaction_.timeout_count++;

         // This transaction will be timeout after 2 seconds 
         // because TFTP_TIMEOUT_LIMIT is 10 and a waiting duration is 1 millisecond.
         // So TFTP_TIMEOUT_LIMIT(10) * 200 is 2000(milliseconds).
        if (transaction_.timeout_count == TFTP_TIMEOUT_LIMIT * 200) {
            fprintf(stderr, "# Timeout.\n");
            operation = TFTP_OPERATION_ABANDONED;

            if (transport_)
                transport_->on_tftp_timeout_transaction();
        }
        else {
            operation = TFTP_OPERATION_FAILED;
        }
    }

    return operation;
}


int TFTPServerTransaction::state_send() {
    int operation = TFTP_OPERATION_DONE;

    if (transport_)
        transport_->tftp_send_data(packet_out_, packet_out_length_);

    if (transaction_.complete == 1) {
        if (transport_) 
            transport_->on_tftp_completed_transaction();
        operation = TFTP_OPERATION_ABANDONED;
    }

    if (transaction_.ecode != TFTP_ECODE_NONE) {
        operation = TFTP_OPERATION_ABANDONED;
    }

    return operation;
}

int TFTPServerTransaction::state_reset() {
    int operation = TFTP_OPERATION_DONE;

    if (transaction_.file_open == 1) {
        if (file_)
            file_->close();

        file_ = nullptr;
    }

    packet_free();
    init_variables();

    return operation;
}
