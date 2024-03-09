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
static const int MAX_RETRY = 10;
static int retryCount = 0;

struct TftpAck{
    uint16_t opcode;
    uint16_t blockNumber;
};

struct TftpData{
    uint16_t opcode;
    uint16_t blockNumber;
    char data[512];
};

struct TftpError{
    uint16_t opcode;
    uint16_t errorCode;
    char errorMessage[1024];
};

// Declare the printBuffer function if needed publicly
void printBuffer(const char *buffer, unsigned int len);

// Struct parsing functions declarations
TftpAck parseAckPacket(const vector<char>& packet);
TftpData parseDataPacket(const vector<char>& packet);
TftpError parseErrorPacket(const vector<char>& packet);

// File operation functions declarations
size_t readFileBlock(ifstream& file, char* buffer, size_t bufferSize);
void writeFileBlock(ofstream& file, const char* data, size_t dataSize);
void handleTimeout(int signum);
int registerTimeoutHandler();

//--------------------------------------
ssize_t sendACK(int sock, const sockaddr_in& clientAddr, uint16_t blockNumber);
ssize_t sendData(int sock, const sockaddr_in& clientAddr, const char* data, size_t dataSize, uint16_t blockNumber);
ssize_t sendError(int sock, const sockaddr_in& clientAddr, int errorCode, const char* errorMessage);
ssize_t sendRequest(int sock, const sockaddr_in& clientAddr, char* packet, int packetLen);
#endif // TFTP_COMMON_H
