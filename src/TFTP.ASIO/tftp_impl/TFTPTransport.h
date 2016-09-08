/* TFTPTransport.h 
   Creaty by Mansoo Kim(mansoo.kim@icloud.com), 2016.8.26

   netudp.h
   Copyright (c) 2013 James Northway
*/

#ifndef TFTP_TRANSPORT
#define TFTP_TRANSPORT

//  TFTPTransport class

struct TFTPTransportEvent {
    //  If you want to process TFTPEvent in your module, you must override following functions.
    //  If you didn't override them, events will be ignored.
    virtual void on_tftp_start_transaction(std::size_t max_block_size) {};
    virtual void on_tftp_progress_transaction(std::size_t current_block_num) {};
    virtual void on_tftp_completed_transaction() {};
    virtual void on_tftp_timeout_transaction() {};
    virtual void on_tftp_close() {};
};

struct TFTPTransport : public TFTPTransportEvent {
    virtual bool tftp_is_connect() = 0;

    //  The module that wants to use TFTPTransaction must implement following functions.
    //  The following functions make to be transparent a releationship between the module and TFTPTransaction to use network resource.
    virtual bool tftp_has_received_data() = 0;
    virtual int tftp_send_data(const char* buf, int size) = 0;
    virtual int tftp_get_received_data(char* buf, const int max_buf_size) = 0;
};

#endif
