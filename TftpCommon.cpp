//
// Created by B Pan on 1/15/24.
//
#include <bits/stdc++.h>
#include <cstdio>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <csignal>
#include <chrono>
#include <thread>
#include "TftpError.h"
#include "TftpOpcode.h"
#include "TftpConstant.h"
using namespace std;

// Helper function to print the first len bytes of the buffer in Hex
static void printBuffer(const char * buffer, unsigned int len) {
    for(int i = 0; i < len; i++) {
        // printf("%x,", buffer[i]);
        printf("%x", buffer[i]);
        if (i < len - 1)
            printf(",");
        else
            printf("\n");
    }
}

static void sendData(int sock, const sockaddr_in& clientAddr, const char* data, size_t dataSize, uint16_t blockNumber) {
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
    sendto(sock, packet, packetSize, 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
}
static void sendACK(int sock, const sockaddr_in& clientAddr, uint16_t blockNumber) {
    char ackPacket[4]; // ACK packet size
    ackPacket[0] = 0; // Opcode for ACK is 0 4
    ackPacket[1] = TFTP_ACK; // Opcode for ACK
    ackPacket[2] = blockNumber >> 8; // Block number high byte
    ackPacket[3] = blockNumber & 0xFF; // Block number low byte
    sendto(sock, ackPacket, sizeof(ackPacket), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
}
static void sendError(int sock, const sockaddr_in& clientAddr, int errorCode, const char* errorMessage) {
    std::vector<char> packet;
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
    sendto(sock, packet.data(), packet.size(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
}

// To track how retransmit/retry has occurred.
static int retryCount = 0;

// Helper function to print the first len bytes of the buffer in Hex

// increment retry count when timeout occurs. 
static void handleTimeout(int signum ){
    retryCount++;
    printf("timeout occurred! count %d\n", retryCount);
}

static int registerTimeoutHandler( ){
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
