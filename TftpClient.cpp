//
// Created by B Pan on 12/3/23.
//

// TFTP client program - CSS 432 - Winter 2024

#include <cstdio>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <stdio.h>
#include "TftpError.h"
#include "TftpOpcode.h"
#include "TftpCommon.cpp"
using namespace std;
#define SERV_UDP_PORT 61125
#define SERV_HOST_ADDR "127.0.0.1"
#define BUFFER_SIZE 1024 
#define DATA_PACKET_SIZE 512

/* A pointer to the name of this program for error reporting.      */
char *program;
unsigned int lastBlockSent = 0;

/* The main program sets up the local socket for communication     */
/* and the server's address and port (well-known numbers) before   */
/* calling the processFileTransfer main loop.                      */

void sendACK(int sock, const sockaddr_in &clientAddr, uint16_t blockNumber)
{
    char ackPacket[4];                 // ACK packet size
    ackPacket[0] = 0;                  // Opcode for ACK is 0 4
    ackPacket[1] = 4;                  // Opcode for ACK
    ackPacket[2] = blockNumber >> 8;   // Block number high byte
    ackPacket[3] = blockNumber & 0xFF; // Block number low byte
    sendto(sock, ackPacket, sizeof(ackPacket), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
}

void sendData(int sock, const sockaddr_in& clientAddr, const char* data, size_t dataSize, uint16_t blockNumber) {
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

void sendError(int sock, const sockaddr_in& clientAddr, int errorCode, const std::string& errorMessage) {
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

bool readNextChunk(FILE *filePtr, char *dataBuffer, size_t *bytesRead)
{
    if (!filePtr || !dataBuffer || !bytesRead)
    {
        // Invalid parameters
        return false;
    }
    // Attempt to read up to 512 bytes from the file
    *bytesRead = fread(dataBuffer, 1, 512, filePtr);
    // return false;
    if (*bytesRead > 0)
    {
        // Data was read successfully
        return true;
    }
    else
    {
        // Check for error or end of file
        if (feof(filePtr))
        {
            // End of file reached
            return false;
        }
        else if (ferror(filePtr))
        {
            // Error reading file
            perror("Error reading file");
            return false;
        }
    }

    // This should not be reached, added to avoid compiler warning
    return false;
}
void handleWRQ(int sockfd, sockaddr_in& serv_addr, FILE* filePtr){

    struct sockaddr_in from_addr = serv_addr;
    socklen_t from_len = sizeof(from_addr);
    unsigned short ack_opcode, ack_block_number;
    char ack_buffer[4]; // ACK packets are 4 bytes: 2 bytes for opcode, 2 bytes for block number

    // Receive ACK packet
    while (1)
    {
        int recv_len = recvfrom(sockfd, ack_buffer, sizeof(ack_buffer), 0, (struct sockaddr *)&from_addr, &from_len);
        if (recv_len < 0)
        {
            perror("Error receiving ACK");
            // Handle error
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
                if (ack_block_number == lastBlockSent)
                {
                    char dataBuffer[512];                        // TFTP DATA packet payload size
                    lastBlockSent = (lastBlockSent + 1) % 65536; // inrease lastBlockSent by 1, if lastBlockSent reaches limitation --> reset to 0
                    // Read the next chunk of data from the file, If it is, and more data exists, prepare and send the next DATA packet
                    if (readNextChunk(filePtr, dataBuffer, &bytesRead))
                    {
                        // Prepare and send the next DATA packet
                        // Note: Ensure lastBlockSent wraps correctly from 65535 to 0 as per TFTP specification
                        char packetBuffer[516]; // DATA packet size: 4 bytes header + 512 bytes data
                        unsigned short *opCode = (unsigned short *)packetBuffer;
                        *opCode = htons(TFTP_DATA); // DATA opcode is 3
                        unsigned short *blockNumber = (unsigned short *)(packetBuffer + 2);
                        *blockNumber = htons(lastBlockSent); // Convert to network byte order

                        memcpy(packetBuffer + 4, dataBuffer, bytesRead); // Copy data into packet

                        // Send the packet
                        if (sendto(sockfd, packetBuffer, bytesRead + 4, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
                        {
                            perror("Failed to send DATA packet");
                        }

                        if (bytesRead < 512)
                        {
                            // This was the last chunk of the file
                            // After sending this DATA packet, wait for the final ACK to conclude the transfer
                            // Wait for the final ACK from the server
                            unsigned short final_ack_opcode, final_ack_block_number;
                            char final_ack_buffer[4]; // ACK packets are 4 bytes: 2 bytes for opcode, 2 bytes for block number

                            if (recvfrom(sockfd, final_ack_buffer, sizeof(final_ack_buffer), 0, (struct sockaddr *)&from_addr, &from_len) < 0)
                            {
                                perror("Error receiving final ACK");
                                // Handle error
                            }
                            else
                            {
                                // Extract opcode and block number from the final ACK
                                memcpy(&final_ack_opcode, final_ack_buffer, 2);
                                final_ack_opcode = ntohs(final_ack_opcode);
                                memcpy(&final_ack_block_number, final_ack_buffer + 2, 2);
                                final_ack_block_number = ntohs(final_ack_block_number);

                                if (final_ack_opcode == TFTP_ACK && final_ack_block_number == lastBlockSent)
                                {
                                    std::cout << endl;
                                }
                                else
                                {
                                    std::cerr << "Unexpected packet received while waiting for final ACK." << std::endl;
                                    cout << endl;
                                    // Handle unexpected packet
                                }
                            }
                            close(sockfd);
                            fclose(filePtr);
                            exit(0); // the last block has size of data < 512 bytes --> exit program
                        }

                        // If no more data, the transfer is complete
                    }
                    else
                    {
                        // No more data to read, transfer is complete
                        std::cout << "File transfer complete." << std::endl;
                        // Close the file and cleanup
                        fclose(filePtr);
                        close(sockfd);
                        // Any other necessary cleanup
                    }
                }
            }
        }
    }
}
void handleRRQ(int sockfd, sockaddr_in& serv_addr, FILE* filePtr){
    struct sockaddr_in from_addr = serv_addr;
    socklen_t from_len = sizeof(from_addr);
    unsigned short ack_opcode, ack_block_number;
    char ack_buffer[4]; 

    int recv_len = recvfrom(sockfd, ack_buffer, sizeof(ack_buffer), 0, (struct sockaddr *)&from_addr, &from_len);
    if (recv_len < 0)
    {
        perror("Error receiving ACK");
        // Handle error
    }
    else if (recv_len == 4)
    { // Proper ACK packet size
        // Extract opcode
        memcpy(&ack_opcode, ack_buffer, 2);
        ack_opcode = ntohs(ack_opcode);

        // Extract block number
        memcpy(&ack_block_number, ack_buffer + 2, 2);
        ack_block_number = ntohs(ack_block_number);
        if (ack_opcode == TFTP_ACK && ack_block_number == 0){
            //server received read request
            char buffer[BUFFER_SIZE];
            int blockNumber = 1;
            size_t recv_len;

            do
            {
                //receive data from server
                recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&from_addr, &from_len);
                if (recv_len < 4)
                { // A valid DATA packet must be at least 4 bytes (opcode + block number)
                    sendError(sockfd, from_addr, 0, "Invalid packet");
                    cout << "recv len < 4\n";
                    break;
                }

                if (buffer[1] == 3)
                { // DATA opcode is 3
                    int receivedBlockNumber = (buffer[2] << 8) | (unsigned char)buffer[3];
                    printf("Reveived block #%d\n", receivedBlockNumber);
                    if (receivedBlockNumber == blockNumber)
                    {
                        string str_to_write = "";
                        // // fileStream.write(buffer + 2, recv_len - 2);
                        for (int i = 4; i <= recv_len - 1; i++)
                            str_to_write += buffer[i];

                        // fileStream << str_to_write;
                        fwrite(buffer+4, sizeof(char), recv_len-4, filePtr);
                        fseek(filePtr, recv_len-4, SEEK_SET);
                        sendACK(sockfd, from_addr, blockNumber);
                        if (recv_len - 4 < DATA_PACKET_SIZE)
                        { // Last packet
                            // fileStream.close();
                            cout << endl;
                            close(sockfd);
                            fclose(filePtr);
                            exit(0);
                            break;
                        }
                        blockNumber++;
                    }
                    else
                    {
                        // Block number mismatch, handle as needed
                        // If the block number is less than expected, it might be a duplicate
                        if (receivedBlockNumber < blockNumber)
                        {
                            // Resend ACK for the duplicate block to confirm receipt
                            sendACK(sockfd, from_addr, receivedBlockNumber);
                        }
                        else
                        {
                            // If the block number is greater than expected, log or handle the error
                            // This situation should not normally occur in TFTP as it implies out-of-order delivery
                            std::cerr << "Received out-of-order block number: " << receivedBlockNumber << " expected: " << blockNumber << std::endl;
                            // Consider sending an error packet to the client and possibly terminating the transfer
                            sendError(sockfd, from_addr, 0, "Unexpected block number received");
                            break; // Exit the loop or handle as appropriate
                        }
                    }
                }
            } while (true);
        }
        
    }

}
int main(int argc, char *argv[])
{
    program = argv[0];

    int sockfd;
    struct sockaddr_in cli_addr, serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(&cli_addr, 0, sizeof(cli_addr));

    /*
     * TODO: initialize server and client address, create socket and bind socket as you did in
     * programming assignment 1
     */
    // create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
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

    // 3. Bind. Associate the socket to serv_addr
    // if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    //     perror("Error on binding");
    //     exit(2);
    // }

    printf("Bind socket successfull\n");
    /*
     * TODO: Verify arguments, parse arguments to see if it is a read request (r) or write request (w),
     * parse the filename to transfer, open the file for read or write
     */

    // Assuming the program requires exactly 2 arguments: operation mode, and filename
    if (argc != 3)
    {
        std::cerr << "Usage: " << program << " <r|w> <filename>" << std::endl;
        exit(1);
    }

    // Extract the operation mode and filename from the arguments
    char operationMode = argv[1][0]; // First character of the first argument ('r' or 'w')

    // char path[strlen(CLIENT_FOLDER) + strlen(argv[2])];
    // strcpy(path, CLIENT_FOLDER);
    // strcat(path, argv[2]);
    const char *filename = argv[2];

    char path[strlen(CLIENT_FOLDER) + strlen(argv[2])];
    strcpy(path, CLIENT_FOLDER);
    strcat(path, argv[2]);
    char *path_to_file = path;
    // Check operation mode and open the file accordingly

    FILE *filePtr = nullptr;
    if (operationMode == 'r')
    {
        // Open the file for reading (server will send the file to the client)
        filePtr = fopen(path_to_file, "at"); //at: for append mode
        // read data from a file of server foler and write to a file in client folder
        // printf("Reading file %s %p\n", path_to_file, filePtr);

        if (!filePtr)
        {
            std::cerr << "Failed to open file for reading: " << path_to_file << std::endl;
            exit(2);
        }
    }
    else if (operationMode == 'w')
    {
        // Open the file for writing (server will receive the file from the client)
        filePtr = fopen(path_to_file, "r");
        // printf("Writing file %s %p\n", path_to_file, filePtr);
        if (!filePtr)
        {
            std::cerr << "Failed to open file for writing: " << path_to_file << std::endl;
            exit(3);
        }
    }
    else
    {
        // Invalid operation mode
        std::cerr << "Invalid operation mode: " << operationMode << ". Use 'r' for read or 'w' for write." << std::endl;
        exit(4);
    }

    /*
     * TODO: create the 1st tftp request packet (RRQ or WRQ) and send it to the server via socket.
     * Remember to use htons when filling the opcode in the tftp request packet.
     *
     * char buffer[MAX_PACKET_LEN];
    char * bpt = buffer; // point to the beginning of the buffer
    Unsigned short * opCode = (unsigned short *) bpt; // opCode points to the beginning of the buffer
    *opCode = htons(TFTP_DATA); // fill in op code for data packet.
    Unsigned short * block = (unsigned short * )(bpt + 2); // move bpt towards right by 2 bytes
    *block = htons(blocknum); // fill in block number
    bpt  = bpt + 4; // move bpt towards right by 4 byes
    // now bpt points to the beginning of the actual file bytes, fill the rest of the 512 bytes of file data using strncpy.
     *
     */

    char buffer[MAX_PACKET_LEN];
    char *bpt = buffer; // Point to the beginning of the buffer

    // Determine the opcode based on the operation mode
    unsigned short *opCode = (unsigned short *)bpt;
    *opCode = htons((operationMode == 'r') ? TFTP_RRQ : TFTP_WRQ); // Set opcode for RRQ or WRQ
    bpt += 2;                                                      // Move pointer right by 2 bytes
    // printf("%p %d\n", bpt, bpt-buffer);
    // Append the filename
    strcpy(bpt, filename);
    bpt += strlen(filename) + 1; // Move pointer right by length of filename + 1 for null byte
    // printf("%p %d\n", bpt, bpt-buffer);

    // Append the mode ("octet" or "netascii")
    const char *mode = "octet"; // or "netascii" depending on your needs
    strcpy(bpt, mode);
    bpt += strlen(mode) + 1; // Move pointer right by length of mode + 1 for null byte
    // printf("%p %d\n", bpt, bpt-buffer);

    // Calculate the packet length
    size_t packetLen = bpt - buffer;
    printf("The request packet is:\n");
    for (int i = 0; i < packetLen; i++)
    {
        printf("%x", buffer[i]);
        if (i < packetLen - 1)
            printf(",");
        else
            printf("\n");
    }

    printf("Processing tftp request...\n");
    // Send the request packet to the server
    if (sendto(sockfd, buffer, packetLen, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Failed to send request");
        exit(5);
    }

    /*
     * TODO: process the file transfer
     */

    /*
     * TODO: Don't forget to close any file that was opened for read/write, close the socket, free any
     * dynamically allocated memory, and necessary clean up.
     */

    if(operationMode == 'w'){
        handleWRQ(sockfd, serv_addr, filePtr);
    }else if (operationMode == 'r'){
        handleRRQ(sockfd, serv_addr, filePtr);
    }
}