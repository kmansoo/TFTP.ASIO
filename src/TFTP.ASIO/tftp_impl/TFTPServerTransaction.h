/* TFTPTransaction.h 
   Copyright (c) 2013 James Northway

   Modified by Mansoo Kim(mansoo.kim@icloud.com), 2016.8.26
*/

#ifndef TFTP_SERVER_TRANSACTION
#define TFTP_SERVER_TRANSACTION

#include <memory>
#include <thread>
#include <mutex>

#include "TFTPTransaction.h"
#include "TFTPFile.h"

//  TFTPServerTransaction class

class TFTPServerTransaction : public TFTPTransaction {
public:
    TFTPServerTransaction(TFTPTransport* transport, const std::string& root_path);
    ~TFTPServerTransaction();

public:
    bool stop();
    void data_event();

private:
    int state_standby();
    int state_send();
    int state_wait();
    int state_receive();
    int state_reset();

private:
    bool is_stop_;

    std::mutex mtx_;
    std::condition_variable cv_;

    std::thread thread_;
};

#endif
