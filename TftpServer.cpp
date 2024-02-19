//
// Created by B Pan on 12/3/23.
//
//for system calls, please refer to the MAN pages help in Linux
//TFTP server program over udp - CSS432 - winter 2024
#include <cstdio>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <unistd.h>
#include "TftpCommon.cpp"
#include <fstream>

using namespace std;
// #define SERV_UDP_PORT 61125
// #define BUFFER_SIZE 1024 // Adjust based on TFTP specifications

char *program;
unsigned int lastBlockSent = 0;

void handleRRQ(int sock, const sockaddr_in& clientAddr, const std::string& fileName) {
    string server_folder (SERVER_FOLDER);
    string file_to_read = server_folder + fileName;
    std::ifstream fileStream(file_to_read, std::ios::binary);
    if (!fileStream) {
        sendError(sock, clientAddr, 5, "File not found");
        return;
    }

    sendACK(sock, clientAddr, 0); //send ack for successfull file name
    // std::vector<char> buffer(DATA_PACKET_SIZE);

    uint16_t blockNumber = 1;

    while (fileStream) {
        std::vector<char> buffer(DATA_PACKET_SIZE); //move to this line
        /*if the buffer has fixed size 512 and the last block has size < 512 (e.g. 182) then it 
        will store the last block for the first 182 bytes and the other bytes (512-182) still 
        contain the data of the last time read before*/
        fileStream.read(buffer.data(), DATA_PACKET_SIZE);
        std::streamsize bytesRead = fileStream.gcount();
        // If bytesRead is 0, we've reached the end of the file. This handles empty files as well.
        if (bytesRead <= 0) {
            char packetBuffer[516]; // DATA packet size: 4 bytes header + 512 bytes data
            unsigned short *opCode = (unsigned short *)packetBuffer;
            *opCode = htons(TFTP_ERROR);
            unsigned short *blockNumber = (unsigned short *)(packetBuffer + 2);
            *blockNumber = htons(lastBlockSent); // Convert to network byte order
            char dataBuffer[512];
            memcpy(packetBuffer + 4, dataBuffer, 0); // Copy data into packet

            // Send the packet
            if (sendto(sock, packetBuffer, bytesRead + 4, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0)
            {
                perror("Failed to send DATA packet");
            }
            break;
        }

        // Convert to using raw pointer for the buffer data
        sendData(sock, clientAddr, buffer.data(), bytesRead, blockNumber++);

        
        // Here you should wait for an ACK for the blockNumber you've just sent
        // If ACK is not received, resend the data or handle error accordingly

        unsigned short ack_opcode, ack_block_number;
        char ack_buffer[4];
        struct sockaddr_in from_addr = clientAddr;
        socklen_t from_len = sizeof(from_addr);
        int recv_len = recvfrom(sock, ack_buffer, sizeof(ack_buffer), 0, (struct sockaddr *)&from_addr, &from_len);
        if (recv_len < 0)
        {
            perror("Error receiving ACK");
            // Handle error
            // break;
        }
        else if (recv_len == 4)
        { // Proper ACK packet size
            // Extract opcode
            memcpy(&ack_opcode, ack_buffer, 2);
            ack_opcode = ntohs(ack_opcode);

            // Extract block number
            memcpy(&ack_block_number, ack_buffer + 2, 2);
            ack_block_number = ntohs(ack_block_number);
            if (ack_opcode == TFTP_ACK)
            {
                // std::cout << "Received ACK for block " << ack_block_number << std::endl;
                printf("Received Ack #%d\n", ack_block_number);
                // Variable to track the last block number sent
                size_t fileSize = 0;  // Variable to track the file size if necessary
                size_t bytesRead = 0; // Variable to track the number of bytes read from the file for the last DATA packet
                // Check if this is the ACK for the last block sent
                if (ack_block_number == lastBlockSent){
                    lastBlockSent++;
                    // break;
                }
            }
        }
    }

    fileStream.close();
}



void handleWRQ(int sock, sockaddr_in& clientAddr, socklen_t cli_len, const std::string& fileName) {
    string server_folder (SERVER_FOLDER);
    string file_to_write = server_folder + fileName;
    std::ofstream fileStream(file_to_write);
    
    if (!fileStream.is_open()) {
        cout <<"ERR\n";
        sendError(sock, clientAddr, 2, "Could not open file for writing");
        return;
    }

    // Send ACK for WRQ (block number 0)
    sendACK(sock, clientAddr, 0);

    char buffer[BUFFER_SIZE];
    int blockNumber = 1;
    ssize_t recv_len;

    do {
        recv_len = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&clientAddr, &cli_len);
        if (recv_len < 4) { // A valid DATA packet must be at least 4 bytes (opcode + block number)
            sendError(sock, clientAddr, 0, "Invalid packet");
            cout << "recv len < 4\n";
            break;
        }

        if (buffer[1] == 3) { // DATA opcode is 3
            int receivedBlockNumber = (buffer[2] << 8) | (unsigned char)buffer[3];
            printf("Reveived block #%d\n", receivedBlockNumber);
            if (receivedBlockNumber == blockNumber) {
                string str_to_write = "";
                // fileStream.write(buffer + 2, recv_len - 2);
                for(int i = 4; i <= recv_len-1; i++) str_to_write += buffer[i];
                fileStream << str_to_write;
                sendACK(sock, clientAddr, blockNumber);
                if (recv_len - 4 < DATA_PACKET_SIZE) { // Last packet
                    break;
                }
                blockNumber++;
            } else {
                // Block number mismatch, handle as needed
                // If the block number is less than expected, it might be a duplicate
                if (receivedBlockNumber < blockNumber) {
                    // Resend ACK for the duplicate block to confirm receipt
                    sendACK(sock, clientAddr, receivedBlockNumber);
                } else {
                    // If the block number is greater than expected, log or handle the error
                    // This situation should not normally occur in TFTP as it implies out-of-order delivery
                    std::cerr << "Received out-of-order block number: " << receivedBlockNumber << " expected: " << blockNumber << std::endl;
                    // Consider sending an error packet to the client and possibly terminating the transfer
                    sendError(sock, clientAddr, 0, "Unexpected block number received");
                    break; // Exit the loop or handle as appropriate
                }
            }
        }else{
            break;
        }
    } while (true);

    fileStream.close();
}

int handleIncomingRequest(int sockfd) {

    struct sockaddr cli_addr;
    /*
     * TODO: define necessary variables needed for handling incoming requests.
     */
    socklen_t cli_len = sizeof(cli_addr);
    char buffer[BUFFER_SIZE];
    FILE *file = nullptr;
    for (;;) {

        printf("\nWating to receive request\n\n");

        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cli_addr, &cli_len);
        if (recv_len < 0) {
            perror("recvfrom failed");
            break;
        }

        char* fileName = buffer + 2;  // Skip the first 2 bytes of opcode
        printf("Requested filename is %s\n", fileName);
        struct sockaddr_in clientAddr = *(struct sockaddr_in*)&cli_addr;
        // Determine packet type (RRQ or WRQ) and handle accordingly
        if (buffer[1] == TFTP_RRQ) {

            handleRRQ(sockfd, clientAddr, std::string(fileName));

        } else if (buffer[1] == TFTP_WRQ) {

            handleWRQ(sockfd, clientAddr, cli_len, std::string(fileName));
        }



        if (file) {
            fclose(file);
            file = nullptr;
        }

        // Additional cleanup if necessary
    }

    return 0;
    
}

int main(int argc, char *argv[]) {
    program=argv[0];

    int sockfd;
    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));

    /*
     * TODO: initialize the server address, create socket and bind the socket as you did in programming assignment 1
     */

    // create a UPD socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Returning on error");
        exit(1);
    }

    // Fill in serv_addr information
    //sets the address family of the server socket
    serv_addr.sin_family = AF_INET;
    // accept calls on any of the server's addresses
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //sets the port number for the socket. 
    serv_addr.sin_port = htons(SERV_UDP_PORT);

    // Bind. Associate the socket to serv_addr
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error on binding");
        exit(2);
    }
    //dg_echo(sockfd);
    handleIncomingRequest(sockfd);

    close(sockfd);
    return 0;
}