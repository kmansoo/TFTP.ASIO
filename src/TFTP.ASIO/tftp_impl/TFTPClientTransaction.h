/* TFTPTransaction.h 
   Copyright (c) 2013 James Northway

   Modified by Mansoo Kim(mansoo.kim@icloud.com), 2016.8.26
*/

#ifndef TFTP_CLIENT_TRANSACTION
#define TFTP_CLIENT_TRANSACTION

#include <memory>
#include <thread>
#include <mutex>

#include "TFTPTransaction.h"

//  TFTPClientTransaction class

class TFTPClientTransaction : public TFTPTransaction {
public:
    TFTPClientTransaction(TFTPTransport* transport);
    ~TFTPClientTransaction();

public:
    bool get_file(const std::string& remoteFilePath, const std::string& localNewFilePath);
    bool put_file(const std::string& localFilePath, const std::string& remoteNewFilePath);

    void data_event();

private:
    std::shared_ptr<std::thread> perform_transaction();

    int state_send();
    int state_wait();
    int state_receive();

private:
    std::mutex mtx_;
    std::condition_variable cv_;
};

#endif
