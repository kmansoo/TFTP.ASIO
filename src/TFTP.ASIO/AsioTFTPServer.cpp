#include <iostream>
#include <functional>

#include <boost/bind.hpp>

#include "AsioTFTPServer.h"

AsioTFTPServer::AsioTFTPServer(boost::asio::io_service& io_service, const std::string& path, const std::string& bind_port) :
    io_service_(io_service), socket_(io_service, udp::endpoint(udp::v4(), std::stoi(bind_port))) {
    packet_in_length_ = 0;
    
    // resolve the host name and port number to an iterator that can be used to connect to the server
    tftp_transaction_ = std::make_shared<TFTPServerTransaction>(this, path);

    remote_endpoint_ = server_endpoint_;

    receive_start();
}

AsioTFTPServer::~AsioTFTPServer() {
    if (socket_.is_open())
        socket_.close();
}

bool AsioTFTPServer::is_open() {
    return socket_.is_open();
}

//
//  implemented functions that are related to TFTPTransport
//
bool AsioTFTPServer::is_connect() {
    return socket_.is_open();
}

bool AsioTFTPServer::has_received_data() {
    if (packet_in_length_ == 0)
        return false;

    return true;
}

int AsioTFTPServer::send_tftp_data(const char* buf, int size) {
    try {
        socket_.async_send_to(
            boost::asio::buffer(buf, size), remote_endpoint_,
            boost::bind(&AsioTFTPServer::handle_send_to, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return size;
}

int AsioTFTPServer::get_received_tftp_data(char* buf, const int max_buf_size) {
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

void AsioTFTPServer::on_completed_transaction()
{
    remote_endpoint_ = server_endpoint_;

    receive_start();
}

//
//  functions related to ASIO
//
void AsioTFTPServer::receive_start() {
    try {
        socket_.async_receive_from(
            boost::asio::buffer(packet_in_buffer_, TFTP_PACKETSIZE), remote_endpoint_,
            boost::bind(&AsioTFTPServer::handle_receive_from, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

void AsioTFTPServer::handle_send_to(const boost::system::error_code& error, size_t bytes_sent) {
    if (!error)
        receive_start();
    else
        std::cout << "# send_handle, error:" << error << std::endl;
}

void AsioTFTPServer::handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd) {
    if (!error && bytes_recvd > 0) {
        packet_in_length_ = bytes_recvd;

        tftp_transaction_->data_event();
    }
    else {
        remote_endpoint_ = server_endpoint_;

        receive_start();
    }
}
