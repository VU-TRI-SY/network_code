//
// Created by B Pan on 1/15/24.
//
#include <bits/stdc++.h>
#include <cstdio>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <vector>
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

/*
 * TODO: Add common code that is shared between your server and your client here. For example: helper functions for
 * sending bytes, receiving bytes, parse opcode from a tftp packet, parse data block/ack number from a tftp packet,
 * create a data block/ack packet, and the common "process the file transfer" logic.
 */

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
    ackPacket[1] = 4; // Opcode for ACK
    ackPacket[2] = blockNumber >> 8; // Block number high byte
    ackPacket[3] = blockNumber & 0xFF; // Block number low byte
    sendto(sock, ackPacket, sizeof(ackPacket), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
}

static void sendError(int sock, const sockaddr_in& clientAddr, int errorCode, const std::string& errorMessage) {
    std::vector<char> packet;
    // Construct and send an ERROR packet similarly to sendData
    // Opcode for ERROR packet
    packet.push_back(0);
    packet.push_back(TFTP_ERROR); // ERROR_OPCODE should be defined as 5

    // Error code
    packet.push_back(errorCode >> 8);
    packet.push_back(errorCode & 0xFF);

    // Error message
    for (char c : errorMessage) {
        packet.push_back(c);
    }
    packet.push_back('\0'); // Null-terminated string

    // Send the packet
    sendto(sock, packet.data(), packet.size(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));

}

