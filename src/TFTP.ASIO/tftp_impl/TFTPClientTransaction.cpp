/* TFTPTransaction.c 
   Copyright (c) 2013 James Northway

   Modified by Mansoo Kim(mansoo.kim@icloud.com), 2016.8.26
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <chrono>

#include "TFTPClientTransaction.h"
#include "TFTPFile.h"

TFTPClientTransaction::TFTPClientTransaction(TFTPTransport* transport) : TFTPTransaction(transport) {
}

TFTPClientTransaction::~TFTPClientTransaction() {
}


bool TFTPClientTransaction::get_file(const std::string& remoteFilePath, const std::string& localNewFilePath) {

    init_variables();

    file_ = std::make_shared<TFTPFile>();

    if ((file_->open_write((char*)localNewFilePath.c_str())) != 0)
    {
        file_ = nullptr;
        return false;
    }

    transaction_.file_open = 1;

    packet_free();
    packet_form_rrq((char*)remoteFilePath.c_str());

    transaction_.blocknum = 1;

    stateOfTFTP_ = TFTP_STATE_SEND;

    auto t = perform_transaction();

    t->join();

    if (transaction_.complete != 1) {
        //  printf("# Transfer failed.\n");
        return false;
    }

    //  printf("# Transfer completed.");

    return true;
}

bool TFTPClientTransaction::put_file(const std::string& localFilePath, const std::string& remoteNewFilePath) {
    init_variables();

    file_ = std::make_shared<TFTPFile>();

    if ((file_->open_read((char*)localFilePath.c_str())) != 0) {
        file_ = nullptr;
        return false;
    }

    transaction_.file_open = 1;

    packet_form_wrq((char*)remoteNewFilePath.c_str());

    stateOfTFTP_ = TFTP_STATE_SEND;

    auto t = perform_transaction();

    t->join();

    if (transaction_.complete != 1) {
        //  printf("# Transfer failed.\n");
        return false;
    }

    //  printf("# Transfer completed.");

    return true;
}

void TFTPClientTransaction::data_event() {
    cv_.notify_one();
}

// STATE FUNCTIONS
std::shared_ptr<std::thread> TFTPClientTransaction::perform_transaction() {
    std::shared_ptr<std::thread> t = std::make_shared<std::thread>([&]() {
        int operation = 0;

        while (stateOfTFTP_ != TFTP_STATE_FINISHED) {

            // We're going to wait for 1 millisecond.
            if (stateOfTFTP_ == TFTP_STATE_WAIT) {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait_for(lk, std::chrono::milliseconds(1));
            }

            switch (stateOfTFTP_) {
            case TFTP_STATE_SEND:
                operation = state_send();
                break;

            case TFTP_STATE_WAIT:
                operation = state_wait();
                break;

            case TFTP_STATE_RECEIVE:
                operation = state_receive();
                break;
            }

            finite_state_machine_client(&stateOfTFTP_, &operation);
        }
    });

    return t;
}

int TFTPClientTransaction::state_send() {
    int operation = TFTP_OPERATION_DONE;

    if (transport_)
        transport_->tftp_send_data(packet_out_, packet_out_length_);

    if (transaction_.complete == 1 || transaction_.ecode != TFTP_ECODE_NONE) {
        operation = TFTP_OPERATION_ABANDONED;
    }

    return operation;
}

int TFTPClientTransaction::state_wait() {
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

int TFTPClientTransaction::state_receive() {

    int operation = -1;

    packet_parse(&packet_, packet_in_buffer_, &packet_in_length_);

    if (TFTP_IS_DATA(packet_.opcode)) {
        operation = packet_receive_data();

    }
    else if (TFTP_IS_ACK(packet_.opcode)) {
        operation = packet_receive_ack();

    }
    else if (TFTP_IS_ERROR(packet_.opcode)) {
        operation = packet_receive_error();

    }
    else {
        operation = packet_receive_invalid();
    }

    return operation;
}
