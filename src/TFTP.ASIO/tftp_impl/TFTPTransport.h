/* TFTPTransport.h 
   Creaty by Mansoo Kim(mansoo.kim@icloud.com), 2016.8.26

   netudp.h
   Copyright (c) 2013 James Northway
*/

#ifndef TFTP_TRANSPORT
#define TFTP_TRANSPORT

//  TFTPTransport class

class TFTPTransport {
public:
    TFTPTransport() {}
    ~TFTPTransport() {}

public:
    virtual bool is_connect() = 0;
    virtual bool has_received_data() = 0;

    virtual int send_tftp_data(const char* buf, int size) = 0;
    virtual int get_received_tftp_data(char* buf, const int max_buf_size) = 0;

    virtual void on_completed_transaction() = 0;
};

#endif
