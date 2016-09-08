/* TFTPFile.h 
   Copyright (c) 2013 James Northway

   Modified by Mansoo Kim(mansoo.kim@icloud.com), 2016.8.26
*/

#ifndef TFTP_FILE
#define TFTP_FILE

#include <memory>
#include <fstream>

#include "TFTPPacket.h"


class TFTPFile {
public:
    TFTPFile();
    ~TFTPFile();

public:
    int open_read(char *filename);
    int open_write(char *filename);

    int read_buffer_from_pos(tftp_transaction_t *transaction);
    int append_from_buffer(tftp_packet_t *packet, tftp_transaction_t *transaction);
    
    std::streamsize get_length();
    std::streampos  get_current_pos();

    int close();

private:
    std::shared_ptr<std::ifstream> read_file_;
    std::shared_ptr<std::ofstream> write_file_;
};

#endif
