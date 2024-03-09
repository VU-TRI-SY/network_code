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
#include "TftpConstant.h"
using namespace std;

// Helper function to print the first len bytes of the buffer in Hex
void printBuffer(const char * buffer, unsigned int len) {
    for(int i = 0; i < len; i++) {
        printf("%x,", buffer[i]);
    }
    printf("\n");
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

ssize_t sendACK(int sock, const sockaddr_in& clientAddr, uint16_t blockNumber){
    char ackPacket[4]; // ACK packet size
    ackPacket[0] = 0; // Opcode for ACK is 0 4
    ackPacket[1] = TFTP_ACK; // Opcode for ACK
    ackPacket[2] = blockNumber >> 8; // Block number high byte
    ackPacket[3] = blockNumber & 0xFF; // Block number low byte
    sendto(sock, ackPacket, sizeof(ackPacket), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
}

ssize_t sendError(int sock, const sockaddr_in& clientAddr, int errorCode, const char* errorMessage) {
    std::vector<char> packet;
    // Construct and send an ERROR packet similarly to sendData
    // Opcode for ERROR packet
    packet.push_back((TFTP_ERROR >> 8) & 0xFF);
    packet.push_back(TFTP_ERROR & 0xFF);

    // Add error code to the packet 
    packet.push_back((errorCode >> 8) & 0xFF);
    packet.push_back(errorCode & 0xFF);

    while (*errorMessage != '\0') {
        packet.push_back(*errorMessage);
        errorMessage++;
    }
    // Add a null terminator to the end of the message
    packet.push_back('\0');
    return sendto(sock, packet.data(), packet.size(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
}

ssize_t sendData(int sock, const sockaddr_in& clientAddr, const char* data, size_t dataSize, uint16_t blockNumber) {
    char packet[4 + DATA_PACKET_SIZE]; // Assuming DATA_PACKET_SIZE is defined as 512
    size_t packetSize = 4 + dataSize; // 4 bytes for header, rest for data

    // Opcode for DATA packet
    packet[0] = 0;
    packet[1] = TFTP_DATA; // DATA_OPCODE should be defined as 3

    // Block number
    packet[2] = blockNumber >> 8;
    packet[3] = blockNumber & 0xFF;

    // Copy data into packet
    memcpy(packet + 4, data, dataSize);

    // Send the packet
    return sendto(sock, packet, packetSize, 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
}

ssize_t sendRequest(int sock, const sockaddr_in& clientAddr, char* packet, int packetLen){
    return sendto(sock, packet, packetLen, 0, (struct sockaddr *)  &clientAddr, sizeof(clientAddr));
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

// increment retry count when timeout occurs. 
void handleTimeout(int signum) {
    retryCount++;
    printf("timeout occurred! count %d\n", retryCount);
}

int registerTimeoutHandler() {
    signal(SIGALRM, handleTimeout);

    /* disable the restart of system call on signal. otherwise the OS will be stuck in
     * the system call
     */
    
    if( siginterrupt( SIGALRM, 1 ) == -1 ){
        printf( "invalid sig number.\n" );
        return -1;
    }
    return 0;
}
