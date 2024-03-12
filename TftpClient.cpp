//
// Created by B Pan on 12/3/23.
//

//TFTP client program - CSS 432 - Winter 2024

#include <cstdio>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include "TftpError.h"
#include "TftpOpcode.h"
#include "TftpCommon.h"
#include "TftpConstant.h"
using namespace std;
//#define SERV_UDP_PORT 61125
//#define SERV_HOST_ADDR "127.0.0.1"

/* A pointer to the name of this program for error reporting.      */
char *program;

void handleWRQ(int sockfd, sockaddr_in& serv_addr, const char* filePath){
    struct sockaddr_in from_addr = serv_addr;
    socklen_t from_len = sizeof(from_addr);
    fstream fileStream;
    fileStream.open(filePath, ios::in | ios::binary); 
    if (!fileStream) {
        cerr << "Error opening file for writing." << endl;
        close(sockfd);
        exit(1);
    }
    vector<char> ackBuffer(BUFFER_SIZE);
    int recv_len = recvfrom(sockfd, ackBuffer.data(), ackBuffer.size(), 0, (struct sockaddr *)&serv_addr, &from_len);
    if(recv_len < 0){
        perror("Initial ACK recvfrom failed");
        close(sockfd);
        fileStream.close();
        exit(1);
    }

    uint16_t recvOpcode = ntohs(*reinterpret_cast<uint16_t*>(ackBuffer.data()));

    if(recvOpcode == TFTP_ERROR){
        TftpError error = exctractError(ackBuffer);
        cout << "Received TFTP Error Packet. Error code " << error.errCode << ". Error Msg: " << error.errMsg << endl;
        close(sockfd);
        exit(1);
    }
    
    //received Ack

    TftpAck ack = exctractAck(ackBuffer);
    if(ack.block != 0){
        cerr << "Error: Initial ACK with block number 0 not received" << endl;
        close(sockfd);
        exit(1);
    }

    cout << "Received ACK #0\n";

    bool lastBlock = false;
    int lastBlockSent = 1;
    size_t bytesRead = 0; // Variable to track the number of bytes read from the file for the last DATA packet
    vector<char> dataBuffer(512);                        // TFTP DATA packet payload size
    while (!lastBlock){
        // Read the next chunk of data from the file, If it is, and more data exists, prepare and send the next DATA packet
            char buffer[512];
            fileStream.read(buffer, sizeof(buffer));
            size_t bytesRead = fileStream.gcount();
            int res = sendData(sockfd, serv_addr, buffer, bytesRead, lastBlockSent);
            if(res < 0) {
                perror("Send data failed.");
                break;
            }

            // this_thread::sleep_for(chrono::milliseconds(200));
            //receive Ack
            recv_len = recvfrom(sockfd, ackBuffer.data(), ackBuffer.size(), 0, (struct sockaddr *)&serv_addr, &from_len);
            if(recv_len < 0){
                perror("ACK recvfrom failed");
                break;
            }

            TftpAck ack = exctractAck(ackBuffer);

            if(lastBlockSent == ack.block){
                cout << "Received ACK #" << lastBlockSent << endl;
                lastBlockSent++;
            }
            if (bytesRead < 512) lastBlock = true;
        
    }
    close(sockfd);
    if(fileStream.is_open()) fileStream.close();
    exit(0);
}

void handleRRQ(int sockfd, sockaddr_in& serv_addr, const char * filePath){
    sockaddr_in from_addr = serv_addr;
    socklen_t from_len = sizeof(serv_addr);
    fstream fileStream;
    fileStream.open(filePath, ios::out | ios::binary); 
    if (!fileStream) {
        cerr << "Error opening file." << endl;
        close(sockfd);
        exit(1);
    }
    vector<char> buffer(BUFFER_SIZE);
    int lastBlockSent = 1;
    size_t recv_len;

    //infinite loop to receive data blocks from server
    bool lastBlock = false;
    while(lastBlock == false){
        //receive data from server
        recv_len = recvfrom(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr *)&serv_addr, &from_len);
        if (recv_len < 0){ 
            perror("Recvfrom failed");
            close(sockfd);
            fileStream.close();
            exit(1);
        }

        uint16_t recv_opCode = ntohs(*reinterpret_cast<uint16_t*>(buffer.data()));

        if(recv_opCode == TFTP_ACK){
            TftpAck ack = exctractAck(buffer);
            int block = ack.block;
            if(block == 0) continue;
        }

        if(recv_opCode == TFTP_ERROR){
            TftpError error = exctractError(buffer);
            cout << "Received TFTP Error Packet. Error code " << error.errCode << ". Error Msg: " << error.errMsg << endl;
            close(sockfd);
            fileStream.close();
            exit(1);
        }

        if(recv_opCode == TFTP_DATA){
            TftpData dataPacket = exctractData(buffer);
            if(dataPacket.block == lastBlockSent){
                fileStream.write(dataPacket.data, recv_len-4);
                cout << "Received Block #" << dataPacket.block << endl;
                lastBlockSent++;
                sendACK(sockfd, serv_addr, dataPacket.block);
                if(recv_len < 516) lastBlock = true;
            }
        }else{
            cerr << "Unexpected opcode received." << endl;
            break;
        }

        // this_thread::sleep_for(chrono::milliseconds(50));

    };
    close(sockfd);
    fileStream.close();
    exit(0);
}
int main(int argc, char *argv[]){
    program = argv[0];

    int sockfd;
    struct sockaddr_in cli_addr, serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(&cli_addr, 0, sizeof(cli_addr));

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Returning on error");
        exit(1);
    }

    // 2. Fill in serv_addr information
    // sets the address family of the server socket
    serv_addr.sin_family = AF_INET;
    // accept calls on any of the server's addresses
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // sets the port number for the socket.
    serv_addr.sin_port = htons(SERV_UDP_PORT);

    printf("Bind socket successfull\n");

    if (argc != 3) {
        std::cerr << "Usage: " << program << " <r|w> <filename>" << std::endl;
        exit(1);
    }

    // Extract the operation mode and filename from the arguments
    char operationMode = argv[1][0]; // First character of the first argument ('r' or 'w')

    const char *filename = argv[2];

    char path[strlen(CLIENT_FOLDER) + strlen(argv[2])];
    strcpy(path, CLIENT_FOLDER);
    strcat(path, argv[2]);
    char *path_to_file = path;
    // Check operation mode and open the file accordingly

    // Determine the opcode 
    uint16_t opcode;
    if (operationMode == 'r') {
        opcode = TFTP_RRQ;
    } else if (operationMode == 'w') {
        opcode = TFTP_WRQ;
    } else {
        opcode = TFTP_UNDEFINED;
    }
    string mode = "octet";

    // Create TFTP request packet 
    vector<char> packet = createRequestPacket(opcode, filename, mode);
    cout << "The request packet is:" << endl;
    printBuffer(packet.data(), packet.size());

    // Send packet to the server 
    sockaddr_in serverAddr;
    socklen_t serverAddrLen = sizeof(serverAddr);
    if (sendto(sockfd, packet.data(), packet.size(), 0, (struct sockaddr*)&serv_addr, serverAddrLen) < 0) {
        perror("Send request failed!");
        close(sockfd);
        exit(1);
    }

    printf("Processing tftp request...\n");
    if(operationMode == 'w'){
        handleWRQ(sockfd, serv_addr, path_to_file);
    }else if(operationMode == 'r'){
        handleRRQ(sockfd, serv_addr, path_to_file);
    }

    //undefined opcode: not receive
}