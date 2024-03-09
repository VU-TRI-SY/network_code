#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fstream>
#include "TftpConstant.h"
#include "TftpOpcode.h"
#include "TftpError.h"
#include "TftpCommon.h"

#define SERV_UDP_PORT 61125
#define SERV_HOST_ADDR "127.0.0.1"

using namespace std;

/* A pointer to the name of this program for error reporting.      */
char *program;

int main(int argc, char *argv[]) {
    program = argv[0];

    struct sockaddr_in cli_addr, serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(&cli_addr, 0, sizeof(cli_addr));

    // Verify arguments 
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " r/w [filename]" << endl;
        exit(1);
    }

    // Parse filename
    char* filename = argv[2];

    // Create socket 
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        cerr << "Error opening socket." << endl;
        exit(1);
    }

    // After creating the socket
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    cli_addr.sin_port = htons(0); // Let OS select the port

    if (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(1);
    } else {
        cout << "Bind successful\n" << endl;
    }

    // Initialize server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_UDP_PORT);
    if (inet_pton(AF_INET, SERV_HOST_ADDR, &serv_addr.sin_addr) <= 0) {
        cerr << "Invalid server address." << endl;
        exit(1);
    }

    // Parse arguments to see if it's read or write request 
    char* requestType = argv[1];
    
    //Create TFTP request packet 
    char buffer[MAX_PACKET_LEN];
    char *bpt = buffer; // Point to the beginning of the buffer

    unsigned short *opCode = (unsigned short *)bpt;
    *opCode = htons((strcmp(requestType, "r") == 0) ? TFTP_RRQ : TFTP_WRQ); // Set opcode for RRQ or WRQ
    bpt += 2;                                                      // Move pointer right by 2 bytes

    strcpy(bpt, filename);
    bpt += strlen(filename) + 1; // Move pointer right by length of filename + 1 for null byte

    // Append the mode ("octet" or "netascii")
    const char *mode = "octet"; // or "netascii" depending on your needs
    strcpy(bpt, mode);
    bpt += strlen(mode) + 1; // Move pointer right by length of mode + 1 for null byte
    // printf("%p %d\n", bpt, bpt-buffer);

    // Calculate the packet length
    size_t packetLen = bpt - buffer;
    printf("The request packet is:\n");

    printBuffer(buffer, packetLen);

    sockaddr_in serverAddr;
    socklen_t serverAddrLen = sizeof(serverAddr);

    if(sendRequest(sockfd, serv_addr, buffer, packetLen) < 0){
        perror("Sendto failed!");
        close(sockfd);
        exit(1);
    }

    // Buffer to hold the response from the server 
    vector<char> recvBuffer(516);
    int blockNumber = 0;
    cout << "\nProcessing tftp Request..." << endl;

    // Open the file for writing (for RRQ) or reading (for WRQ)
    fstream fileStream;
    // Process received packet
    uint16_t receivedOpcode = ntohs(*reinterpret_cast<uint16_t*>(recvBuffer.data()));
    // Check if the response is an error packet 
    if (receivedOpcode == TFTP_ERROR) {
        TftpError errorPacket = parseErrorPacket(recvBuffer);
        cout << "Received TFTP Error Packet. Error code " << errorPacket.errorCode << ". Error Msg: " << errorPacket.errorMessage << endl;
        close(sockfd);
        exit(1);
    }

    if (strcmp(requestType, "r") == 0) {
        string filenameStr(filename); // Convert char* to string
        string filePath = string(CLIENT_FOLDER) + filenameStr;
        // Open the file for writing as we are going to write the data received from the server
        fileStream.open(filePath, ios::out | ios::binary); 
        if (!fileStream) {
            cerr << "Error opening file for writing." << endl;
            close(sockfd);
            exit(1);
        }

        // Main loop for RRQ to receive data from the server
        bool lastPacket = false;

        while (!lastPacket) {
            // Receive data packet
            int recvLen = recvfrom(sockfd, recvBuffer.data(), recvBuffer.size(), 0, (struct sockaddr*)&serverAddr, &serverAddrLen);

            if (recvLen < 0) {
                perror("Recvfrom failed");
                close(sockfd);
                exit(1);
            }

            // Process received packet
            uint16_t receivedOpcode1 = ntohs(*reinterpret_cast<uint16_t*>(recvBuffer.data()));

            if(receivedOpcode1 == TFTP_ACK){
                TftpAck ackPacket = parseAckPacket(recvBuffer); 
                int blockNumber = ackPacket.blockNumber;
                if(blockNumber == 0) continue;
            } 

            // Check if the response is an error packet 
            if (receivedOpcode1 == TFTP_ERROR) {
                TftpError errorPacket = parseErrorPacket(recvBuffer);
                cout << "Received TFTP Error Packet. Error code " << errorPacket.errorCode << ". Error Msg: " << errorPacket.errorMessage << ": " << string(SERVER_FOLDER) << filename << endl;
                close(sockfd);
                exit(1);
            }

            if (receivedOpcode1 == TFTP_DATA) {
                TftpData dataPacket = parseDataPacket(recvBuffer);
                if (dataPacket.blockNumber == blockNumber + 1) {
                    // Write data to file
                    fileStream.write(dataPacket.data, recvLen - 4);
                    cout << "Received Block #" << dataPacket.blockNumber << endl;
                    blockNumber++;

                    sendACK(sockfd, serverAddr, dataPacket.blockNumber);

                    if (recvLen < 516) { // Last packet
                        lastPacket = true;
                    }
                }
            } else {
                cerr << "Unexpected opcode received." << endl;
                break;
            }
        }
    } else if (strcmp(requestType, "w") == 0) {
        string filenameStr(filename); // Convert char* to string
        string filePath = string(CLIENT_FOLDER) + filenameStr;
        fileStream.open(filePath, ios::in | ios::binary);
        if (!fileStream) {
            cerr << "Error opening file for reading or file is empty." << endl;
            close(sockfd);
            exit(1);
        }
        
        // Wait for the initial ACK from the server
        vector<char> ackBuffer(512);
        int recvLen = recvfrom(sockfd, ackBuffer.data(), ackBuffer.size(), 0, (struct sockaddr*)&serv_addr, &serverAddrLen);
        if (recvLen < 0) {
            perror("Initial ACK recvfrom failed");
            close(sockfd);
            exit(1);
        }

        // Process received packet
        uint16_t receivedOpcode1 = ntohs(*reinterpret_cast<uint16_t*>(ackBuffer.data()));
        // Check if the response is an error packet 
        if (receivedOpcode1 == TFTP_ERROR) {
            TftpError errorPacket = parseErrorPacket(ackBuffer);
            cout << "Received TFTP Error Packet. Error code " << errorPacket.errorCode << ". Error Msg: " << errorPacket.errorMessage << ": " << string(SERVER_FOLDER) << filename << endl;
            close(sockfd);
            exit(1);
        }

        TftpAck initialAck = parseAckPacket(ackBuffer);
        if (initialAck.blockNumber != 0) {
            cerr << "Error: Initial ACK with block number 0 not received" << endl;
            close(sockfd);
            exit(1);
        }
        cout << "Received ACK #0\n";

        // Now handle sending the file blocks
        int blockNumber = 1; // Start with block number 1 for the data packets
        bool lastPacket = false;
        char buffer[512];

        while (!lastPacket) {    
            fileStream.read(buffer, sizeof(buffer));
            size_t bytesRead = fileStream.gcount();

            if(sendData(sockfd, serv_addr, buffer, bytesRead, blockNumber) < 0){
                perror("Data send to failed");
                break;
            }

            // Add a delay of 200 milliseconds 
            this_thread::sleep_for(chrono::milliseconds(350));


            // Wait for ACK for each data packet sent
            recvLen = recvfrom(sockfd, ackBuffer.data(), ackBuffer.size(), 0, (struct sockaddr*)&serv_addr, &serverAddrLen);
            if (recvLen < 0) {
                perror("ACK recvfrom failed");
                break;
            }

            TftpAck ackPacket = parseAckPacket(ackBuffer);
            if (ackPacket.blockNumber != blockNumber) {
                // cerr << "Received incorrect ACK number. Expected: " << blockNumber << " Received: " << ackPacket.blockNumber << endl;
                // break;
            }else{ //ackPacket.blockNumber == blockNumber
                cout << "Received ACK #" << ackPacket.blockNumber << endl;
                blockNumber++;
            }
            if (bytesRead < 512) {
                lastPacket = true;
            }

        } 
    }

    // Clean up
    if (fileStream.is_open()) {
        fileStream.close();
    }
    close(sockfd);
    exit(0);
}