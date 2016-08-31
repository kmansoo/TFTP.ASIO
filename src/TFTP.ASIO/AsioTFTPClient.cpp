#include <iostream>
#include <functional>

#include <boost/bind.hpp>

#include "AsioTFTPClient.h"

//  [Mansoo Kim]
//
//  This TFTPClient supports ASYNC_MODE and SYNC_MODE when it download or uploads a file from TFTP Server.
//  So you can choose mode by setting TFTP_CLIENT_SYNC_MODE_ENABLE in AsioTFTPClient.cpp when you build this project.
//  The default mode is ASYNC_MODE.
//
//  I had confirmed that SYNC_MODE was faster than ASYNC_MODE.

//  #define TFTP_CLIENT_SYNC_MODE_ENABLE 1

AsioTFTPClient::AsioTFTPClient(boost::asio::io_service& io_service, const std::string& dest_ip, const std::string& dest_port) :
    io_service_(io_service), socket_(io_service) {
    packet_in_length_ = 0;
    
    // resolve the host name and port number to an iterator that can be used to connect to the server
    socket_.open(udp::v4());

    udp::resolver resolver(io_service_);
    udp::resolver::query query(udp::v4(), dest_ip, dest_port);
    udp::resolver::iterator iterator = resolver.resolve(query);

    server_endpoint_ = *iterator;
}

AsioTFTPClient::~AsioTFTPClient() {
    bye();
}

bool AsioTFTPClient::bye() {
    if (socket_.is_open())
        socket_.close();

    return true;
}

bool AsioTFTPClient::is_open() {
    return socket_.is_open();
}

bool AsioTFTPClient::is_working() {
    if (tftp_transaction_ == nullptr)
        return false;

    return (tftp_transaction_->transaction_.file_open == 1) ? true : false;
}

bool AsioTFTPClient::is_transfer_completed() {
    if (tftp_transaction_ == nullptr)
        return false;

    return (tftp_transaction_->transaction_.complete == 1) ? true : false;
}


// [WARNING!]
//
//  by Mansoo Kim
//  At this time, I don't consider async operation for getting a file with TFTP Server.
//  If you want to process async operation, you must implemet thread when 'tftp_transaction_->get_file()' is called.
bool AsioTFTPClient::get_file(const std::string& remoteFilePath, const std::string& localNewFilePath) {
    if (tftp_transaction_ == nullptr)
        tftp_transaction_ = std::make_shared<TFTPClientTransaction>(this);

    remote_endpoint_ = server_endpoint_;

    return tftp_transaction_->get_file(remoteFilePath, localNewFilePath);
}

// [WARNING!]
//
//  by Mansoo Kim
//  At this time, I don't consider async operation for putting a file with TFTP Server.
//  If you want to process async operation, you must implemet thread when 'tftp_transaction_->put_file()' is called.
bool AsioTFTPClient::put_file(const std::string& localFilePath, const std::string& remoteNewFilePath) {
    if (tftp_transaction_ == nullptr)
        tftp_transaction_ = std::make_shared<TFTPClientTransaction>(this);

    remote_endpoint_ = server_endpoint_;

    return tftp_transaction_->put_file(localFilePath, remoteNewFilePath);
}

//
//  implemented functions that are related to TFTPTransport
//
bool AsioTFTPClient::is_connect() {
    return socket_.is_open();
}


bool AsioTFTPClient::has_received_data() {

#ifdef TFTP_CLIENT_SYNC_MODE_ENABLE
    if (packet_in_length_ == 0) {
        try {
            packet_in_length_ = socket_.receive_from(
                boost::asio::buffer(packet_in_buffer_, TFTP_PACKETSIZE),
                remote_endpoint_);
        }
        catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }
#endif // TFTP_CLIENT_SYNC_MODE_ENABLE

    if (packet_in_length_ == 0)
        return false;

    return true;
}

int AsioTFTPClient::send_tftp_data(const char* buf, int size) {
    try {

#ifdef TFTP_CLIENT_SYNC_MODE_ENABLE
        socket_.send_to(boost::asio::buffer(buf, size), remote_endpoint_);
#else
        socket_.async_send_to(
            boost::asio::buffer(buf, size), remote_endpoint_,
            boost::bind(&AsioTFTPClient::handle_send_to, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
#endif
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return size;
}

int AsioTFTPClient::get_received_tftp_data(char* buf, const int max_buf_size) {
    // [WARNING!]
    //
    //  by Mansoo Kim
    //  At this time, I don't consider about remanent data after copying user buffer from received buffer.
    //  So remanent data is going to be lost. If you want to process remanent data, you must modify this source.
    int copy_size = packet_in_length_;

    if (copy_size > max_buf_size)
        copy_size = max_buf_size;

    memcpy(buf, packet_in_buffer_, copy_size);

    packet_in_length_ = 0;

    return copy_size;
}

//
//  functions related to ASIO
//
void AsioTFTPClient::receive_start() {

    try {
        socket_.async_receive_from(
            boost::asio::buffer(packet_in_buffer_, TFTP_PACKETSIZE), remote_endpoint_,
            boost::bind(&AsioTFTPClient::handle_receive_from, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

void AsioTFTPClient::handle_send_to(const boost::system::error_code& error, size_t bytes_sent) {
    if (!error)
        receive_start();
    else
        std::cout << "# send_handle, error:" << error << std::endl;
}

void AsioTFTPClient::handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd) {
    if (!error && bytes_recvd > 0) {
        packet_in_length_ = bytes_recvd;

        tftp_transaction_->data_event();
    }
}
