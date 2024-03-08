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
#include "TftpCommon.cpp"

//#define SERV_UDP_PORT 61125
//#define SERV_HOST_ADDR "127.0.0.1"

/* A pointer to the name of this program for error reporting.      */
char *program;
void exitProgram(int sockfd, FILE* filePtr, int exit_code){
    fclose(filePtr);
    close(sockfd);
    exit(exit_code);
}

int receiveACK(int sockfd, sockaddr_in& serv_addr){
    struct sockaddr_in from_addr = serv_addr;
    socklen_t from_len = sizeof(from_addr);
    unsigned short ack_opcode, ack_block_number;
    char ack_buffer[4]; 
    int recv_len = recvfrom(sockfd, ack_buffer, sizeof(ack_buffer), 0, (struct sockaddr *)&from_addr, &from_len);
    if (recv_len < 0) {
        perror("Error receiving ACK");
        return -1;
    } 

    memcpy(&ack_opcode, ack_buffer, 2);
    ack_opcode = ntohs(ack_opcode);

    // Extract block number
    memcpy(&ack_block_number, ack_buffer + 2, 2);
    ack_block_number = ntohs(ack_block_number);
    if (ack_opcode == TFTP_ACK) {
        return ack_block_number;
    }

    if(ack_opcode == TFTP_ERROR){
        int errCode = (ack_buffer[2] << 8) | (unsigned char)ack_buffer[3];
        string err_msg = "";
        for (int i = 4; i <= recv_len - 1; i++)
            err_msg += ack_buffer[i];

        cout << "Received TFTP Error Packet. Error code " << errCode << ". Error Msg: " << err_msg << endl;
    } else cerr << "Unexpected opcode received." << endl;

    return -1;
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

    int res = receiveACK(sockfd, serv_addr);
    if(res > 0) {
        cerr << "Error: Initial ACK with block number 0 not received" << endl;
        exitProgram(sockfd, filePtr, 1);
    }

    if(res < 0) exitProgram(sockfd, filePtr, 1);
    printf("Received Ack #0\n");

    bool lastBlock = false;
    int lastBlockSent = 1;
    size_t bytesRead = 0; // Variable to track the number of bytes read from the file for the last DATA packet
    char dataBuffer[512];                        // TFTP DATA packet payload size
    while (!lastBlock)
    {
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
            if (sendto(sockfd, packetBuffer, bytesRead + 4, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
                perror("Failed to send DATA packet");
                // exitProgram(sockfd, filePtr, 1);
                break;
            }

            this_thread::sleep_for(chrono::milliseconds(400));
            int recv_res = receiveACK(sockfd, serv_addr);
            if(recv_res < 0) break;
            if(lastBlockSent == recv_res) cout << "Received ACK #" << lastBlockSent << endl;
            else {
                while(true){
                    if (sendto(sockfd, packetBuffer, bytesRead + 4, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
                        perror("Failed to send DATA packet");
                        exitProgram(sockfd, filePtr, 1);
                    }
                    this_thread::sleep_for(chrono::milliseconds(400));
                    int recv_res = receiveACK(sockfd, serv_addr);
                    if(recv_res < 0) exitProgram(sockfd, filePtr, 1);
                    if(lastBlockSent == recv_res){
                        cout << "Received ACK #" << lastBlockSent << endl;
                        break;
                    }
                }

            }
            if (bytesRead < 512) lastBlock = true;
            lastBlockSent++;
        }else break;
    }
    exitProgram(sockfd, filePtr, 0);
}
void handleRRQ(int sockfd, sockaddr_in& serv_addr, FILE* filePtr){
    struct sockaddr_in from_addr = serv_addr;
    socklen_t from_len = sizeof(from_addr);
    //receive the first ACK for confirming from server 
    // int res = receiveACK(sockfd, serv_addr);
    // if(res != 0) exitProgram(sockfd, filePtr, 1);

    char buffer[BUFFER_SIZE];
    int blockNumber = 1;
    size_t recv_len;

    //infinite loop to receive data blocks from server
    bool lastBlock = false;
    while(lastBlock == false)
    {
        //receive data from server
        recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&from_addr, &from_len);
        if (recv_len < 0)
        { // A valid DATA packet must be at least 4 bytes (opcode + block number)
            // sendError(sockfd, from_addr, 0, "Invalid packet");
            perror("Recvfrom failed");
            exitProgram(sockfd, filePtr, 1);
        }

        if (buffer[1] == TFTP_DATA) { // DATA opcode is 3
            int receivedBlockNumber = (buffer[2] << 8) | (unsigned char)buffer[3];
            if (receivedBlockNumber == blockNumber)
            {
                printf("Reveived block #%d\n", receivedBlockNumber);
                string str_to_write = "";
                // // fileStream.write(buffer + 2, recv_len - 2);
                for (int i = 4; i <= recv_len - 1; i++)
                    str_to_write += buffer[i];

                // fileStream << str_to_write;
                fwrite(buffer+4, sizeof(char), recv_len-4, filePtr);
                fseek(filePtr, recv_len-4, SEEK_SET);
                sendACK(sockfd, from_addr, blockNumber);
                blockNumber++;
                if(recv_len < 516) lastBlock = true;
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
        } else if(buffer[1] == TFTP_ERROR){
            int errCode = (buffer[2] << 8) | (unsigned char)buffer[3];
            string err_msg = "";
            // // fileStream.write(buffer + 2, recv_len - 2);
            for (int i = 4; i <= recv_len - 1; i++)
                err_msg += buffer[i];
            cout << "Received TFTP Error Packet. Error code " << errCode << ". Error Msg: " << err_msg << endl;
            exitProgram(sockfd, filePtr, 1);
        } else{
            cerr << "Unexpected opcode received." << endl;
            break;
        }
    };
    exitProgram(sockfd, filePtr, 0);
}
int main(int argc, char *argv[])
{
    program = argv[0];

    int sockfd;
    struct sockaddr_in cli_addr, serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(&cli_addr, 0, sizeof(cli_addr));

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


    printf("Bind socket successfull\n");

    if (argc != 3)
    {
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

    FILE *filePtr = nullptr;
    if (operationMode == 'r')
    {
        // Open the file for reading (server will send the file to the client)
        filePtr = fopen(path_to_file, "at"); //open file in client-folder

        if (!filePtr)
        {
            std::cerr << "Failed to open file " << path_to_file << std::endl;
            close(sockfd);
            exit(1);
        }
    }
    else if (operationMode == 'w')
    {
        // Open the file for writing (server will receive the file from the client)
        filePtr = fopen(path_to_file, "r");
        // printf("Writing file %s %p\n", path_to_file, filePtr);
        if (!filePtr)
        {
            std::cerr << "Failed to open file: " << path_to_file << std::endl;
            close(sockfd);
            exit(1);
        }
    }
    else
    {
        // Invalid operation mode
        std::cerr << "Invalid operation mode: " << operationMode << ". Use 'r' for read or 'w' for write." << std::endl;
        exit(1);
    }

    char buffer[MAX_PACKET_LEN];
    char *bpt = buffer; // Point to the beginning of the buffer

    // Determine the opcode based on the operation mode
    unsigned short *opCode = (unsigned short *)bpt;
    *opCode = htons((operationMode == 'r') ? TFTP_RRQ : TFTP_WRQ); // Set opcode for RRQ or WRQ
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


    // Send the request packet to the server
    if (sendto(sockfd, buffer, packetLen, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Failed to send request");
        exit(1);
    }
    printf("Processing tftp request...\n");
    if(operationMode == 'w'){
        handleWRQ(sockfd, serv_addr, filePtr);
    }else if (operationMode == 'r'){
        handleRRQ(sockfd, serv_addr, filePtr);
    }
}