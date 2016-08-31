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
    ~AsioTFTPServer();

public:
    bool    is_open();

protected:
    virtual bool is_connect();
    virtual bool has_received_data();
    virtual int send_tftp_data(const char* buf, int size);
    virtual int get_received_tftp_data(char* buf, const int max_buf_size);
    virtual void on_completed_transaction();

private:
    void    receive_start();

    void    handle_receive_from(const boost::system::error_code& error, std::size_t);
    void    handle_send_to(const boost::system::error_code& error, std::size_t);

private:
    boost::asio::io_service& io_service_;
    udp::socket socket_;                    // the socket this instance is connected to

    udp::endpoint server_endpoint_;
    udp::endpoint remote_endpoint_;

    std::shared_ptr<TFTPServerTransaction> tftp_transaction_;

    char  packet_in_buffer_[TFTP_PACKETSIZE];
    int   packet_in_length_;
};

#endif
