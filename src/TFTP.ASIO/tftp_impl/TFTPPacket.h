/* 
    TFTPPacketDefinition.h
    Copyright (c) 2013 James Northway

    Modified by Mansoo Kim(mansoo.kim@icloud.com), 2016.8.26
*/

#ifndef TFTP_PACKET_DEFINITION
#define TFTP_PACKET_DEFINITION

#define TFTP_BLIMIT      512 /*Maximum amount of file contents that can be buffered*/
#define TFTP_PACKETSIZE  516 /*Total size of TFTP packet*/

typedef unsigned short opcode_t; /*TFTP Operation Code*/
#define TFTP_OPCODE_RRQ   1
#define TFTP_IS_RRQ(op)   ((op) == TFTP_OPCODE_RRQ) /*Evaluates as true or false*/
#define TFTP_OPCODE_WRQ   2
#define TFTP_IS_WRQ(op)   ((op) == TFTP_OPCODE_WRQ)
#define TFTP_OPCODE_DATA  3
#define TFTP_IS_DATA(op)  ((op) == TFTP_OPCODE_DATA)
#define TFTP_OPCODE_ACK   4
#define TFTP_IS_ACK(op)   ((op) == TFTP_OPCODE_ACK)
#define TFTP_OPCODE_ERROR 5
#define TFTP_IS_ERROR(op) ((op) == TFTP_OPCODE_ERROR)

typedef unsigned short ecode_t; /*TFTP Error code*/
#define TFTP_ECODE_NONE  8
#define TFTP_ECODE_0     0
#define TFTP_IS_ECODE_0(ec)  ((ec) == TFTP_ECODE_0)
#define TFTP_ESTRING_0   "Not defined, see error message(if any)."
#define TFTP_ECODE_1     1
#define TFTP_IS_ECODE_1(ec)  ((ec) == TFTP_ECODE_1)
#define TFTP_ESTRING_1   "File not found."
#define TFTP_ECODE_2     2
#define TFTP_IS_ECODE_2(ec)  ((ec) == TFTP_ECODE_2)
#define TFTP_ESTRING_2   "Access violation."
#define TFTP_ECODE_3     3
#define TFTP_IS_ECODE_3(ec)  ((ec) == TFTP_ECODE_3)
#define TFTP_ESTRING_3   "Disk full or allocation exceeded."
#define TFTP_ECODE_4     4
#define TFTP_IS_ECODE_4(ec)  ((ec) == TFTP_ECODE_4)
#define TFTP_ESTRING_4   "Illegal TFTP operation."
#define TFTP_ECODE_5     5
#define TFTP_IS_ECODE_5(ec)  ((ec) == TFTP_ECODE_5)
#define TFTP_ESTRING_5   "Unknown transfer ID."
#define TFTP_ECODE_6     6
#define TFTP_IS_ECODE_6(ec)  ((ec) == TFTP_ECODE_6)
#define TFTP_ECODE_7     7
#define TFTP_IS_ECODE_7(ec)  ((ec) == TFTP_ECODE_7)
#define TFTP_ESTRING_7   "No such user."

                                /*Transfer operation modes*/
#define TFTP_MODE_NETASCII "netascii" 
#define TFTP_MODE_OCTET    "octet"
#define TFTP_MODE_MAIL     "mail"

#define TFTP_TIMEOUT         1 /*Socket receive timeout*/
#define TFTP_TIMEOUT_LIMIT   10

typedef unsigned short tftp_bnum_t; /*TFTP Block number, for data packets*/
typedef char tftp_packetbuffer_t; /*Packet buffer*/

                                  /*Struct for storage of the data extracted from a packet buffer*/
typedef struct tftp_packet {
    char filename[TFTP_PACKETSIZE];
    opcode_t opcode;
    char mode[TFTP_PACKETSIZE];
    char data[TFTP_BLIMIT];
    int data_length;
    tftp_bnum_t blocknum;
    ecode_t ecode;
    char estring[TFTP_PACKETSIZE];
    int estring_length;
} tftp_packet_t;

/*Struct for storage of data relating to the current transfer*/
typedef struct tftp_transaction {
    unsigned last_ack;
    int timeout_count;
    int timed_out;
    int bad_packet_count;
    int final_packet;
    int complete;
    int rebound_socket;
    int tid;
    int file_open;
    int filepos;
    //  int filedata;   //  by Mansoo Kim, this variable didn't use in this modified version.
    char filebuffer[TFTP_BLIMIT];
    int filebuffer_length;
    char mode[TFTP_PACKETSIZE];
    tftp_bnum_t blocknum;
    ecode_t ecode;
    char estring[64];
    int estring_length;
} tftp_transaction_t;


#endif


