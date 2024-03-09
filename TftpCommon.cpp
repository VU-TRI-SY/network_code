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
#include "TftpCommon.h"

using namespace std;

// Helper function to print the first len bytes of the buffer in Hex
void printBuffer(const char * buffer, unsigned int len) {
    for(int i = 0; i < len; i++) {
        printf("%x,", buffer[i]);
    }
    printf("\n");
}

// Function to create an RRQ and WRQ packet 
vector<char> createRequestPacket(uint16_t opcode, const string& filename, const string& mode) {
    // Hold the packet data
    vector<char> packet;

    // Add opcode to the packet 
    packet.push_back((opcode >> 8) & 0xFF); // High byte of opcode
    packet.push_back(opcode & 0xFF); // Low byte of opcode 

    // Add filename to the packet 
    for (char c : filename) {
        packet.push_back(c);
    }
    // Add a null terminator to the end of the filename 
    packet.push_back('\0');

    // Add mode for the packet 
    for (char c : mode) {
        packet.push_back(c);
    }
    // ADd a null terminator to the end of the mode 
    packet.push_back('\0');

    return packet;
}

// Function to create ACK packet 
vector<char> createAckPacket(uint16_t blockNumber) {
    vector<char> packet;

    // Add opcode for ACK
    packet.push_back((TFTP_ACK >> 8) & 0xFF);
    packet.push_back(TFTP_ACK & 0xFF);

    // Add block number
    packet.push_back((blockNumber >> 8) & 0xFF);
    packet.push_back(blockNumber & 0xFF);

    return packet;
}

TftpAck parseAckPacket(const vector<char>& packet) {
    // Create an ACK struct 
    TftpAck ack;

    // Copy the opcode from the packet to the struct 
    memcpy(&ack.opcode, packet.data(), sizeof(ack.opcode));
    // Convert from netwoek byte order to host byte order
    ack.opcode = ntohs(ack.opcode);

    // Copy the block number from the packet to the struct 
    memcpy(&ack.blockNumber, packet.data() + sizeof(ack.opcode), sizeof(ack.blockNumber));
    // Convert 
    ack.blockNumber = ntohs(ack.blockNumber);

    return ack;
}

vector<char> createDataPacket(uint16_t blockNumber, const char* data, size_t dataSize) {
    vector<char> packet;

    // Add opcode to the packet 
    packet.push_back((TFTP_DATA >> 8) & 0xFF);
    packet.push_back(TFTP_DATA & 0xFF);

    // Add block number to the packet
    packet.push_back((blockNumber >> 8) & 0xFF);
    packet.push_back(blockNumber & 0xFF);

    // Add data to the packet 
    packet.insert(packet.end(), data, data + dataSize);

    return packet;
}

TftpData parseDataPacket(const vector<char>& packet) {
    TftpData dataPacket;

    // Copy the opcode from the packet to the struct 
    memcpy(&dataPacket.opcode, packet.data(), sizeof(dataPacket.opcode));
    // Convert from netwoek byte order to host byte order
    dataPacket.opcode = ntohs(dataPacket.opcode);

    // Copy the block number from the packet to the struct 
    memcpy(&dataPacket.blockNumber, packet.data() + sizeof(dataPacket.opcode), sizeof(dataPacket.blockNumber));
    // Convert 
    dataPacket.blockNumber = ntohs(dataPacket.blockNumber);

    // Extract the data 
    memcpy(dataPacket.data, packet.data() + 4, packet.size() - 4);

    return dataPacket;
}

// Function to create an ERROR packet
vector<char> createErrorPacket(uint16_t errorCode, const char* errorMsg) {
    vector<char> packet;

    // Add opcode to the packet
    packet.push_back((TFTP_ERROR >> 8) & 0xFF);
    packet.push_back(TFTP_ERROR & 0xFF);

    // Add error code to the packet 
    packet.push_back((errorCode >> 8) & 0xFF);
    packet.push_back(errorCode & 0xFF);

    // Add error message to the packet
    while (*errorMsg != '\0') {
        packet.push_back(*errorMsg++);
    }
    // Add a null terminator to the end of the message
    packet.push_back('\0');

    return packet;
}

// Function to parse an ERROR packet
TftpError parseErrorPacket(const vector<char>& packet) {
    TftpError errorPacket;

    // Copy the opcode from the packet to the struct
    memcpy(&errorPacket.opcode, packet.data(), sizeof(errorPacket.opcode));
    errorPacket.opcode = ntohs(errorPacket.opcode); // Convert from network byte order to host byte order

    // Copy the error code from the packet to the struct
    memcpy(&errorPacket.errorCode, packet.data() + sizeof(errorPacket.opcode), sizeof(errorPacket.errorCode));
    errorPacket.errorCode = ntohs(errorPacket.errorCode); // Convert from network byte order to host byte order

    // Extract the error message (starts after opcode and error code)
    const char* startOfMsg = reinterpret_cast<const char*>(packet.data() + 4);
    strncpy(errorPacket.errorMessage, startOfMsg, sizeof(errorPacket.errorMessage) - 1);
    errorPacket.errorMessage[sizeof(errorPacket.errorMessage) - 1] = '\0'; // Ensure null-termination

    return errorPacket;
}

// Function to read a block of data from a file 
size_t readFileBlock(ifstream& file, char* buffer, size_t bufferSize) {
    file.read(buffer, bufferSize);
    // Return number of the bytes read
    return file.gcount();
}

// Function to write a block of data to a file 
void writeFileBlock(ofstream& file, const char* data, size_t dataSize) {
    file.write(data, dataSize);
}

