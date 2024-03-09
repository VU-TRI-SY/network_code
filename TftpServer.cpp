#include <cstdio>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <ctime>
#include <sstream>
#include "TftpConstant.h"
#include "TftpOpcode.h"
#include "TftpError.h"
#include "TftpCommon.h"

using namespace std;

char *program = nullptr;

string extractFileName(const char* buffer, size_t length) {
    // The file name is a null-terminated string starting at buffer.
    string filename(buffer);
    return string(buffer);
}

string extractMode(const char* buffer, size_t length) {
    // Mode also follows the filename and is a null-terminated string.
    // It should start after the filename plus its null terminator.
    const char *modePtr = buffer + strlen(buffer) + 1;
    string mode(modePtr);
    return string(mode);
}

void sendFileData(int sockfd, ifstream& fileStream, const sockaddr_in& cli_addr, socklen_t cli_len) {
    //send ACK to confirm
    sendACK(sockfd, cli_addr, 0); //init ACK

    int result = registerTimeoutHandler();
    if (result < 0) {
        cerr << "Failed to register timeout handler" << endl;
        return;
    }

    // Logic for reading file contents and sending DATA packets.
    // If the file is larger than 512 bytes, need to send multiple packets.
    uint16_t blockNumber = 1;
    bool lastPacket = false;
    // ACK is 4 bytes: 2 bytes for opcode, 2 bytes for block number
    char fileBuffer[512];
    int count = retryCount;

    while (!lastPacket) {
        // Read a block of data from the file 
        fileStream.read(fileBuffer, sizeof(fileBuffer));
        size_t bytesRead = fileStream.gcount();
        // If less than 512, it's the last packet
        lastPacket = bytesRead < 512;
 
        while (true) {
            // Set timer for 1 second
            alarm(1); 
            sendData(sockfd, cli_addr, fileBuffer, bytesRead, blockNumber);
            vector<char> ackBuffer(4);
            ssize_t ackLen = recvfrom(sockfd, ackBuffer.data(), ackBuffer.size(), 0, (struct sockaddr*)&cli_addr, &cli_len);
            // After receiving ACK packet successfully, clear timer
            alarm(0);

            // Check for errors in recvfrom
            if (ackLen > 0) {
                TftpAck ackPacket = parseAckPacket(ackBuffer);
                if (ackPacket.blockNumber == blockNumber) {
                    cout << "Received Ack #" << ackPacket.blockNumber << endl;
                    blockNumber++;
                    retryCount = 0;
                    break;
                } 

            } else if (errno == EINTR) {
                // Timeout Occured
                if (retryCount > MAX_RETRY) {
                    cerr << "Max retransmission reached. Aborting transfer." << endl;
                    return;
                }

                cout << "Retransmitting #" << blockNumber << endl;
                // Retransmit the packet
                sendData(sockfd, cli_addr, fileBuffer, bytesRead, blockNumber);
            } else {
                cerr << "recvfrom error: " << strerror(errno) << endl;
                return;
            }
        }
        // Add a delay of 200 milliseconds 
        // this_thread::sleep_for(chrono::milliseconds(350));
    }
    alarm(0); 
}

void receiveFileData(int sockfd, ofstream& fileStream, const sockaddr_in& cli_addr, socklen_t cli_len) {
    int result = registerTimeoutHandler();
    if (result < 0) {
        cerr << "Failed to register timeout handler" << endl;
        return;
    }

    // Receiving DATA packets and writing to the file.
    sendACK(sockfd, cli_addr, 0);

    uint16_t blockNumber = 1;
    char buffer[MAX_PACKET_LEN];
    int count = retryCount;

    while (true) {
        // Clear buffer for the incoming packet.
        memset(buffer, 0, sizeof(buffer));
        // Set alarm
        alarm(1);
        int receivedBytes = recvfrom(sockfd, buffer, MAX_PACKET_LEN, 0, (struct sockaddr*)&cli_addr, &cli_len);
        // Cancel the alarm immediately after recvfrom
        alarm(0);

        if (receivedBytes >= 4) {
            // Extract block number from received data.
            uint16_t receivedBlockNumber = ntohs(*(uint16_t*)(buffer + 2)); 

            if (receivedBlockNumber == blockNumber) {
                // If the block number is what we expected, write the data to the file.
                fileStream.write(buffer + 4, receivedBytes - 4); // Write data, excluding the 4-byte header.
                blockNumber++;

                sendACK(sockfd, cli_addr, receivedBlockNumber);
                
                // Reset the timeout count after successfully receiving data
                retryCount = 0;
                cout << "Received Block #" << receivedBlockNumber << endl;
                
                // Check if it's the last packet (less than 512 bytes of data).
                if (receivedBytes < MAX_PACKET_LEN) {
                    break; 
                }
            } 
        } else if (errno == EINTR) {
            // Timeout occurred, increment count and send the last ACK again.
            if (retryCount > MAX_RETRY) {
                cerr << "Max retransmission reached. Transfer failed." << endl;
                return;
            }

            // Resend the last ACK 
            sendACK(sockfd, cli_addr, blockNumber);
            cout << "Retransmitting ACK #" << blockNumber << endl;

        } else if (receivedBytes < 0) {
            // Some other error occurred.
            cerr << "recvfrom error: " << strerror(errno) << endl;
            break; // Exit on other errors.
        }
        // this_thread::sleep_for(chrono::milliseconds(350));
    }
}

int handleIncomingRequest(int sockfd) {
    sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    // Buffer to hold incoming packets
    char buffer[1024];

    cout << "TFTP Server started. Waiting for client connections on port " << SERV_UDP_PORT << "." << endl;

    for (;;) {
        cout << "\nWaiting to receive request...\n" << endl;
        // Clear the buffer for the incoming packet 
        memset(buffer, 0, sizeof(buffer));
        ssize_t receivedBytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, &cli_len);
        if (receivedBytes <  0) {
            perror("Fail Receive");
            // Skip the rest of the loop iteration on error
            continue;
        }
        
        // Parse the opcode from the packet
        // First two bytes of the buffer
        uint16_t opcode = ntohs(*(uint16_t *)buffer);
 
        // TODO: process the file transfer
        switch(opcode) {
            case TFTP_RRQ: {
                // Handle Read request
                // Extracting fileame and mode from the request
                // Assuming buffer contains the request right after the opcode
                string filename = extractFileName(buffer + 2, receivedBytes - 2);
                // Construct the full path to the file
                string filePath = string(SERVER_FOLDER) + filename;
                string mode = extractMode(buffer + 2 + filename.length() + 1, receivedBytes - 2 - filename.length() - 1);
                cout << "Read request (RRQ) for file: " << filePath << endl;

                // Open the file for reading 
                ifstream fileStream(filePath, ios::binary);
                if (!fileStream) {
                    // Send error pack if can't open
                    cerr << "Error: File not Found!" << endl;
                    sendError(sockfd, cli_addr, TFTP_ERROR_FILE_NOT_FOUND, "File not found");
                } else {
                    // Send the file data in blocks of 512 bytes
                    sendFileData(sockfd, fileStream, *(struct sockaddr_in*)&cli_addr, cli_len);
                }
                break;
            }

            case TFTP_WRQ: {
                // Extract filename and mode from the request
                string filename = extractFileName(buffer + 2, receivedBytes - 2);
                string mode = extractMode(buffer + 2 + filename.length() + 1, receivedBytes - 2 - filename.length() - 1);
                string filePath = string(SERVER_FOLDER) + filename;
                cout << "Write request (WRQ) for file: " << filePath << endl;
                ifstream testStream(filePath, ios::binary); //use ifstream to open file with read mode --> check file exist
                // Check if the file is already exist 
                if (testStream.good()) {
                    cerr << "Error: File already exists!" << endl;
                    sendError(sockfd, cli_addr, TFTP_ERROR_FILE_ALREADY_EXISTS, "File already exists");
                    testStream.close();
                    break;
             
                } else {
                    // Proceed to open the new or renamed file for writing
                    ofstream fileStream(filePath, ios::out | ios::binary);
                    if (!fileStream) {
                        cerr << "Error: Cannot open file for writing." << endl;
                        sendError(sockfd, cli_addr, TFTP_ERROR_INVALID_ARGUMENT_COUNT, "Cannot open file");
                    } else {
                        // Receive the file data from the client
                        receiveFileData(sockfd, fileStream, *(struct sockaddr_in*)&cli_addr, cli_len);
                    }
                    break;
                }
            }

            default: {
                string filename = extractFileName(buffer + 2, receivedBytes - 2);
                cout << "Requested filename is: " << filename << endl;
                cerr << "Error: Received Illegal Opcode: " << opcode << endl;
                // Create an error packet for an illegal TFTP operation
                sendError(sockfd, cli_addr, TFTP_ERROR_INVALID_OPCODE, "Received Illegal Opcode");
                break;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    program=argv[0];

    int sockfd;
    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));

    // Create socket for server 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Cannot open socket.");
        exit(1);
    }

    // Prepare the sockaddr_in structure
    serv_addr.sin_family = AF_INET;
    // Server should be reachable at any IP
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
    serv_addr.sin_port = htons(SERV_UDP_PORT);

    // Bind the socket to the server address
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed!");
        exit(1);
    }

    // Call function to handle incoming requests
    handleIncomingRequest(sockfd);

    // Close the socket before program termination
    close(sockfd);
    return 0;
}
