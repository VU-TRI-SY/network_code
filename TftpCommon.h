//
// Created by B Pan on 1/15/24.
//

#ifndef TFTP_COMMON_H
#define TFTP_COMMON_H

#include <cstdio>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <csignal>
#include <chrono>
#include <thread>
#include <string>
#include <cstring>
#include <vector> 
#include <fstream>
#include <cstdint> 
#include "TftpError.h"
#include "TftpOpcode.h"

using namespace std;

struct TftpAck{
    uint16_t opcode;
    uint16_t block;
};

struct TftpData{
    uint16_t opcode;
    uint16_t block;
    char data[512];
};

struct TftpError{
    uint16_t opcode;
    uint16_t errCode;
    char errMsg[512];
};

// Helper function to print the first len bytes of the buffer in Hex
void printBuffer(const char * buffer, unsigned int len);

vector<char> createRequestPacket(uint16_t opcode, const string& filename, const string& mode);

ssize_t sendData(int sock, const sockaddr_in& clientAddr, const char* data, size_t dataSize, uint16_t blockNumber);
ssize_t sendACK(int sock, const sockaddr_in& clientAddr, uint16_t blockNumber) ;
ssize_t sendError(int sock, const sockaddr_in& clientAddr, int errorCode, const char* errorMessage);

TftpAck exctractAck (vector<char> packet);
TftpData exctractData (vector<char> packet);
TftpError exctractError (vector<char> packet);


// To track how retransmit/retry has occurred.
static int retryCount = 0;

// Helper function to print the first len bytes of the buffer in Hex

// increment retry count when timeout occurs. 
void handleTimeout(int signum );

int registerTimeoutHandler( );

/*
 * Useful things:
 * alarm(1) // set timer for 1 sec
 * alarm(0) // clear timer
 * std::this_thread::sleep_for(std::chrono::milliseconds(200)); // slow down transmission
 */


/*
 * TODO: Add common code that is shared between your server and your client here. For example: helper functions for
 * sending bytes, receiving bytes, parse opcode from a tftp packet, parse data block/ack number from a tftp packet,
 * create a data block/ack packet, and the common "process the file transfer" logic.
 */
#endif
