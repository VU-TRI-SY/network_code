//
// Created by B Pan on 1/15/24.
//
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
        printf("%x", buffer[i]);
        if (i < len - 1) printf(",");
    }
    printf("\n");
}

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

ssize_t sendData(int sock, const sockaddr_in& clientAddr, const char* data, size_t dataSize, uint16_t blockNumber) {
    vector<char> packet;

    packet.push_back((TFTP_DATA >> 8) & 0xFF);
    packet.push_back(TFTP_DATA & 0xFF);

    packet.push_back((blockNumber >> 8) & 0xFF);
    packet.push_back(blockNumber & 0xFF);

    packet.insert(packet.end(), data, data + dataSize);
    return sendto(sock, packet.data(), packet.size(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
}
ssize_t sendACK(int sock, const sockaddr_in& clientAddr, uint16_t blockNumber) {
    vector<char> packet;

    // Add opcode for ACK
    packet.push_back((TFTP_ACK >> 8) & 0xFF);
    packet.push_back(TFTP_ACK & 0xFF);

    // Add block number
    packet.push_back((blockNumber >> 8) & 0xFF);
    packet.push_back(blockNumber & 0xFF);
    
    return sendto(sock, packet.data(), packet.size(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
}
ssize_t sendError(int sock, const sockaddr_in& clientAddr, int errorCode, const char* errorMessage) {
    vector<char> packet;
    // Construct and send an ERROR packet similarly to sendData
    // Opcode for ERROR packet
    packet.push_back((TFTP_ERROR >> 8) & 0xFF);
    packet.push_back(TFTP_ERROR & 0xFF);

    // Add error code to the packet 
    packet.push_back((errorCode >> 8) & 0xFF);
    packet.push_back(errorCode & 0xFF);
    while (*errorMessage != '\0') {
        packet.push_back(*errorMessage++);
    }
    // Add a null terminator to the end of the message
    packet.push_back('\0');
    return sendto(sock, packet.data(), packet.size(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
}

TftpAck exctractAck (vector<char> packet){
    TftpAck ack;
    memcpy(&ack.opcode, packet.data(), sizeof(ack.opcode));
    ack.opcode = ntohs(ack.opcode);

    memcpy(&ack.block, packet.data() + sizeof(ack.opcode), sizeof(ack.block));
    ack.block = ntohs(ack.block);

    return ack;
}

TftpData exctractData (vector<char> packet){
    TftpData data;
    memcpy(&data.opcode, packet.data(), sizeof(data.opcode)); 
    data.opcode = ntohs(data.opcode);

    memcpy(&data.block, packet.data() + sizeof(data.opcode), sizeof(data.block));
    data.block = ntohs(data.block);

    memcpy(data.data, packet.data() + 4, packet.size()-4);
    return data;
}

TftpError exctractError (vector<char> packet){
    TftpError error;

    memcpy(&error.opcode, packet.data(), sizeof(error.opcode)); 
    error.opcode = ntohs(error.opcode);

    memcpy(&error.errCode, packet.data() + sizeof(error.opcode), sizeof(error.errCode));
    error.errCode = ntohs(error.errCode);

    const char* startOfMsg = reinterpret_cast<const char*>(packet.data() + 4);
    strncpy(error.errMsg, startOfMsg, sizeof(error.errMsg) - 1);
    error.errMsg[sizeof(error.errMsg) - 1] = '\0'; 
    
    return error;
}

void handleTimeout(int signum ){
    retryCount++;
    printf("timeout occurred! count %d\n", retryCount);
}

int registerTimeoutHandler( ){
    signal(SIGALRM, handleTimeout);

    /* disable the restart of system call on signal. otherwise the OS will be stuck in
     * the system call
     */
    //pass 1 for second parameter of siginterrupt() to interrupt system call when SIG happens
    if( siginterrupt( SIGALRM, 1 ) == -1 ){
        printf( "invalid sig number.\n" );
        return -1;
    }
    return 0;
}
