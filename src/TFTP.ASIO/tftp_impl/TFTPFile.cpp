/* TFTPFile.c 
   Copyright (c) 2013 James Northway

   Modified by Mansoo Kim(mansoo.kim@icloud.com), 2016.8.26
*/

#include "TFTPFile.h"

TFTPFile::TFTPFile() {

}

TFTPFile::~TFTPFile() {
    close();
}

int TFTPFile::open_read(char *filename) {
    read_file_ = std::make_shared<std::ifstream>(filename, std::ifstream::binary);

    if (read_file_->is_open() == false)
        return -1;
        
    return 0;
}

int TFTPFile::open_write(char *filename) {
    write_file_ = std::make_shared<std::ofstream>(filename, std::ofstream::binary);

    if (write_file_->is_open() == false)
        return -1;

    return 0;
}

int TFTPFile::read_buffer_from_pos(tftp_transaction_t *transaction) {
    if (read_file_ == nullptr)
        return -1;

    read_file_->seekg(transaction->filepos);   
    read_file_->read((char*)transaction->filebuffer, TFTP_BLIMIT);

    return (int)read_file_->gcount();
}

int TFTPFile::append_from_buffer(tftp_packet_t *packet, tftp_transaction_t *transaction) {

    if (write_file_ == nullptr)
        return -1;

    write_file_->write(packet->data, packet->data_length);

    return 0;
}

int TFTPFile::close() {
    if (read_file_) {
        read_file_->close();
        read_file_ = nullptr;
    }

    if (write_file_) {
        write_file_->close();
        write_file_ = nullptr;
    }

    return 0;
}

std::streamsize TFTPFile::get_length() {
    std::streamsize length = 0;

    if (read_file_)
    {
        // get length of file:
        std::streampos cur_pos = read_file_->tellg();

        read_file_->seekg(0, read_file_->end);
        length = read_file_->tellg();

        read_file_->seekg(cur_pos, read_file_->cur);
    }

    if (write_file_)
        length = write_file_->tellp();

    return length;
}

std::streampos TFTPFile::get_current_pos() {
    std::streampos cur_pos = 0;

    if (read_file_)
        cur_pos = read_file_->tellg();

    if (write_file_)
        cur_pos = write_file_->tellp();

    return cur_pos;
}
