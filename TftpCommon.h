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

// Function declarations for creating and parsing packets
vector<char> createRequestPacket(uint16_t opcode, const string& filename, const string& mode);
vector<char> createAckPacket(uint16_t blockNumber);
vector<char> createDataPacket(uint16_t blockNumber, const char* data, size_t dataSize);
vector<char> createErrorPacket(uint16_t errorCode, const char* errorMsg);

// Struct parsing functions declarations
TftpAck parseAckPacket(const vector<char>& packet);
TftpData parseDataPacket(const vector<char>& packet);
TftpError parseErrorPacket(const vector<char>& packet);

// File operation functions declarations
size_t readFileBlock(ifstream& file, char* buffer, size_t bufferSize);
void writeFileBlock(ofstream& file, const char* data, size_t dataSize);

#endif // TFTP_COMMON_H
