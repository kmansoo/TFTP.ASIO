#ifndef __ASIO_TFTP_SERVER_H__
#define __ASIO_TFTP_SERVER_H__

#include <string>
#include <thread>

#include <boost/asio.hpp>

#include "./tftp_impl/TFTPServerTransaction.h"
#include "./tftp_impl/TFTPTransport.h"

using boost::asio::ip::udp;

class AsioTFTPServer : public TFTPTransport {
public:
    AsioTFTPServer(boost::asio::io_service& io_service, const std::string& path, const std::string& bind_port);
    AsioTFTPServer(TFTPTransportEvent* event_handler, boost::asio::io_service& io_service, const std::string& path, const std::string& bind_port);

    ~AsioTFTPServer();

public:
    bool    close();
    bool    is_open();

protected:
    virtual bool tftp_is_connect();
    virtual bool tftp_has_received_data();
    virtual int tftp_send_data(const char* buf, int size);
    virtual int tftp_get_received_data(char* buf, const int max_buf_size);

    virtual void on_tftp_start_transaction(std::size_t max_block_size);
    virtual void on_tftp_progress_transaction(std::size_t current_block_num);
    virtual void on_tftp_completed_transaction();
    virtual void on_tftp_timeout_transaction();

private:
    void    receive_start();

    void    handle_receive_from(const boost::system::error_code& error, std::size_t);
    void    handle_send_to(const boost::system::error_code& error, std::size_t);

private:
    bool is_server_stop;

    boost::asio::io_service& io_service_;
    udp::socket socket_;                    // the socket this instance is connected to

    udp::endpoint server_endpoint_;
    udp::endpoint remote_endpoint_;

    TFTPTransportEvent* transport_event_handler;
    std::shared_ptr<TFTPServerTransaction> tftp_transaction_;

    char  packet_in_buffer_[TFTP_PACKETSIZE];
    int   packet_in_length_;

    int max_block_size_;
};

#endif
