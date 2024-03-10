/*
 * TODO: Add constant variables in this file
 */
static const unsigned int MAX_PACKET_LEN = 516; // data 512 + opcode 2 + block num 2
static const int MAX_RETRY_COUNT = 10;
static const char * SERVER_FOLDER = "server-files/"; // DO NOT CHANGE
static const char * CLIENT_FOLDER = "client-files/"; // DO NOT CHANGE
static int DATA_PACKET_SIZE = 512;
static int SERV_UDP_PORT = 61125;
static std::string SERV_HOST_ADDR = "127.0.0.1";
static int BUFFER_SIZE = 516;