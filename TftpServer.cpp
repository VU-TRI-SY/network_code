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
#include "TftpCommon.h"
#include "TftpConstant.h"
#include <fstream>

using namespace std;

char *program;
unsigned int lastBlockSent = 0;

void handleRRQ(int sock, const sockaddr_in& clientAddr, const std::string& fileName) {
    string server_folder (SERVER_FOLDER);
    string file_to_read = server_folder + fileName;
    std::ifstream fileStream(file_to_read, std::ios::binary);
    if (!fileStream) {
        sendError(sock, clientAddr, TFTP_ERROR_FILE_NOT_FOUND, "File not found");
        return;
    }

    sendACK(sock, clientAddr, 0); //send ack for successfull file name
    // std::vector<char> buffer(DATA_PACKET_SIZE);
    int result = registerTimeoutHandler();
    if (result < 0) {
        cerr << "Failed to register timeout handler" << endl;
        return;
    }

    uint16_t blockNumber = 1;
    bool lastBlock = false;
    vector<char> dataBuffer(DATA_PACKET_SIZE); //move to this line
    while (lastBlock == false) {
        fileStream.read(dataBuffer.data(), DATA_PACKET_SIZE);
        size_t bytesRead = fileStream.gcount();
        lastBlock = (bytesRead < 512);

        // sendData(sock, clientAddr, buffer.data(), bytesRead, blockNumber++);
    
        while(true){
            unsigned short ack_opcode, ack_block_number;
            vector<char> ack_buffer(4);
            struct sockaddr_in from_addr = clientAddr;
            socklen_t from_len = sizeof(from_addr);
            alarm(1);
            sendData(sock, clientAddr, dataBuffer.data(), bytesRead, blockNumber);
            int recv_len = recvfrom(sock, ack_buffer.data(), ack_buffer.size(), 0, (struct sockaddr *)&from_addr, &from_len);
            alarm(0);
            this_thread::sleep_for(chrono::milliseconds(400));
            if (recv_len > 0) { // Proper ACK packet size
                TftpAck ack = exctractAck(ack_buffer);
                if(ack.block == blockNumber){
                    cout << "Received Ack #" << ack.block << endl;
                    blockNumber++;
                    retryCount = 0;
                    break;
                }
            }else if(errno == EINTR){
                if (retryCount > MAX_RETRY_COUNT) {
                    cerr << "Max retransmission reached. Aborting transfer." << endl;
                    fileStream.close();
                    return;
                }

                cout << "Retransmitting #" << blockNumber << endl;
                // Retransmit the packet
                sendData(sock, clientAddr, dataBuffer.data(), bytesRead, blockNumber);
            } else{
                cerr << "recvfrom error: " << strerror(errno) << endl;
                fileStream.close();
                return;
            }
        }
    }
    alarm(0);
    fileStream.close();
}

void handleWRQ(int sock, sockaddr_in& clientAddr, socklen_t cli_len, const std::string& fileName) {
    string server_folder (SERVER_FOLDER);
    string file_to_write = server_folder + fileName;
    ifstream testStream(file_to_write, ios::binary);
    if(testStream.good()){
        //error:  TFTP_ERROR_FILE_ALREADY_EXISTS
        sendError(sock, clientAddr, TFTP_ERROR_FILE_ALREADY_EXISTS, "File already exists");
        testStream.close();
        return; //stop function 
    }
    
    std::ofstream fileStream(file_to_write, ios::out | ios::binary); //open file to write
    if (!fileStream.is_open()) {
        sendError(sock, clientAddr, TFTP_ERROR_INVALID_ARGUMENT_COUNT, "Could not open file");
        return;
    }

    int result = registerTimeoutHandler();
    if (result < 0) {
        cerr << "Failed to register timeout handler" << endl;
        return;
    }

    // Send ACK for WRQ (block number 0)
    sendACK(sock, clientAddr, 0);

    vector<char> dataBuffer(BUFFER_SIZE);
    int blockNumber = 1;
    size_t recv_len;

    while(true) {
        memset(dataBuffer.data(), 0, dataBuffer.size());
        alarm(1);
        recv_len = recvfrom(sock, dataBuffer.data(), dataBuffer.size(), 0, (struct sockaddr*)&clientAddr, &cli_len);
        alarm(0);
        if (recv_len < 0) { // A valid DATA packet must be at least 4 bytes (opcode + block number)
            cerr << "recvfrom error: " << strerror(errno) << endl;
            break; // Exit on other errors.
        }

        if (recv_len >= 4) { 
            TftpData dataPacket = exctractData(dataBuffer);
            if(dataPacket.block == blockNumber){
                fileStream.write(dataBuffer.data() + 4, recv_len-4);
                blockNumber++;
                sendACK(sock, clientAddr, dataPacket.block);
                retryCount = 0;
                cout << "Received Block #" << dataPacket.block << endl;

                if(recv_len < MAX_PACKET_LEN) break;
            }
        }else if(errno == EINTR){
            if (retryCount > MAX_RETRY_COUNT) {
                cerr << "Max retransmission reached. Transfer failed." << endl;
                fileStream.close();
                return;
            }

            sendACK(sock, clientAddr, blockNumber);
            cout << "Retransmitting ACK #" << blockNumber << endl;
        }
    };

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
            continue;
        }

        char* fileName = buffer + 2;  // Skip the first 2 bytes of opcode
        printf("Requested filename is %s\n", fileName);
        struct sockaddr_in clientAddr = *(struct sockaddr_in*)&cli_addr;
        // Determine packet type (RRQ or WRQ) and handle accordingly
        if (buffer[1] == TFTP_RRQ) handleRRQ(sockfd, clientAddr, std::string(fileName));
        else if (buffer[1] == TFTP_WRQ) handleWRQ(sockfd, clientAddr, cli_len, std::string(fileName));

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
        exit(1);
    }
    //dg_echo(sockfd);
    handleIncomingRequest(sockfd);

    close(sockfd);
    return 0;
}