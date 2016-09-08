#include <iostream>
#include <functional>

#include <boost/bind.hpp>

#include "AsioTFTPServer.h"

AsioTFTPServer::AsioTFTPServer(boost::asio::io_service& io_service, const std::string& path, const std::string& bind_port) :
    is_server_stop(true), io_service_(io_service), socket_(io_service), transport_event_handler(nullptr), max_block_size_(0) {
    packet_in_length_ = 0;
    
    // resolve the host name and port number to an iterator that can be used to connect to the server
    tftp_transaction_ = std::make_shared<TFTPServerTransaction>(this, path);

    std::string port = bind_port;

    if (bind_port.length() == 0)
        port = "69";

    try {

        socket_.open(udp::v4());
        socket_.bind(udp::endpoint(udp::v4(), std::stoi(port)));

        is_server_stop = false;

        remote_endpoint_ = server_endpoint_;

        receive_start();
    }
    catch (std::exception& e) {        
        socket_.close();

        std::cerr << "Exception: " << e.what() << "\n";
        std::cerr << "Please check what your port(" << port << ") was used by another application." << "\n";
    }
}

AsioTFTPServer::AsioTFTPServer(TFTPTransportEvent* event_handler, boost::asio::io_service& io_service, const std::string& path, const std::string& bind_port) : 
    AsioTFTPServer(io_service, path, bind_port) {
    transport_event_handler = event_handler;
}


AsioTFTPServer::~AsioTFTPServer() {
    close();
}

bool AsioTFTPServer::close() {
    if (is_server_stop)
        return true;

    is_server_stop = true;

    if (socket_.is_open())
        socket_.close();

    if (tftp_transaction_) {
        tftp_transaction_->stop();
        tftp_transaction_ = nullptr;
    }

    if (transport_event_handler)
        transport_event_handler->on_tftp_close();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    return true;
}

bool AsioTFTPServer::is_open() {
    return socket_.is_open();
}

//
//  implemented functions that are related to TFTPTransport
//
bool AsioTFTPServer::tftp_is_connect() {
    return socket_.is_open();
}

bool AsioTFTPServer::tftp_has_received_data() {
    if (packet_in_length_ == 0)
        return false;

    return true;
}

int AsioTFTPServer::tftp_send_data(const char* buf, int size) {
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

int AsioTFTPServer::tftp_get_received_data(char* buf, const int max_buf_size) {
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

void AsioTFTPServer::on_tftp_start_transaction(std::size_t max_block_size) {
    max_block_size_ = max_block_size;

    if (transport_event_handler)
        transport_event_handler->on_tftp_start_transaction(max_block_size);
}

void AsioTFTPServer::on_tftp_progress_transaction(std::size_t current_block_num) {
    if (transport_event_handler)
        transport_event_handler->on_tftp_progress_transaction(current_block_num);
}

void AsioTFTPServer::on_tftp_completed_transaction()
{
    remote_endpoint_ = server_endpoint_;

    receive_start();

    if (transport_event_handler)
        transport_event_handler->on_tftp_completed_transaction();
}

void AsioTFTPServer::on_tftp_timeout_transaction() {
    if (transport_event_handler)
        transport_event_handler->on_tftp_timeout_transaction();
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
    if (is_server_stop)
        return;

    if (!error) {
        if (bytes_recvd > 0) {
            packet_in_length_ = bytes_recvd;

            tftp_transaction_->data_event();
        }
    }
    else
        close();
}
