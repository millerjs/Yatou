#include <limits>
#include <cstddef>
#include <arpa/inet.h>
#include <iostream>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// IP
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |Version|  IHL  |Type of Service|          Total Length         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |         Identification        |Flags|      Fragment Offset    |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |  Time to Live |    Protocol   |         Header Checksum       |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       Source Address                          |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                    Destination Address                        |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                    Options                    |    Padding    |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


#define DEBUG true
#define DEFAULT_MAX_CONNECTIONS 1024

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;


// ======================================================================
// Error handling

class Error : std::exception {
private:
    std::string message_;
public:
    Error(std::string message) : message_(message) { }
    std::string toString() { return message_; }
};

// ======================================================================
// TCP
//
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |          Source Port          |       Destination Port        |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                        Sequence Number                        |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                    Acknowledgment Number                      |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |  Data |           |U|A|P|R|S|F|                               |
// | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
// |       |           |G|K|H|T|N|N|                               |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |           Checksum            |         Urgent Pointer        |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                    Options                    |    Padding    |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                             data                              |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

typedef struct tcp_header {
    unsigned short  source;
    unsigned short  dest;
    unsigned int    seq;
    unsigned int    ack_seq;
    unsigned short  doff: 4,
        res1: 4,
        cwr: 1,
        ece: 1,
        urg: 1,
        ack: 1,
        psh: 1,
        rst: 1,
        syn: 1,
        fin: 1;
    unsigned  short  window;
    unsigned short  check;
    unsigned short  urg_ptr;
    unsigned int    options: 24,
        padding: 8;
} tcp_header_t;


typedef struct tcp_data {
        tcp_header_t*   header;
        unsigned char*  data;
        unsigned int    data_len;
} tcp_data_t;

// ======================================================================
// Global setup


namespace Yatou {
    typedef uint16_t port_t;
    typedef uint32_t socket_t;

    int socket_;
    std::map<port_t, socket_t> sockets;

    void setup(in_addr_t address, u32 port);
    int socket();
    int teardown();

};

void Yatou::setup(in_addr_t address, u32 port)
{
    // don't initialize twice
    if (socket_ > 0) {
        return;
    }

    // create yatou socket
    if ((socket_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) <= 0) {
        throw Error(std::string("Unable to create yatou socket: ")
                    + strerror(errno));
    } else {
        std::cout << "created yatou socket" << std::endl;
    }

    // construct address info
    struct sockaddr_in address_info;
    memset(&address_info, 0, sizeof(address_info));
    address_info.sin_family = AF_INET;
    address_info.sin_addr.s_addr = htonl(address);
    address_info.sin_port = htons(port);
    struct sockaddr* addr_info = ((struct sockaddr*)& address_info);

    // bind yatou socket
    if (::bind(socket_, addr_info, sizeof(address_info)) < 0) {
        throw Error(std::string("Unable to bind yatou socket ")
                    + strerror(errno));
    } else {
        std::cout << "bound yatou socket " << std::endl;
    }
}

// int Yatou::socket()
// {
//     socket_t next_socket;
//     for (next_socket = 0;
//          next_socket < std::numeric_limits<socket_t>::max();
//          next_socket++) {

//     }
// }


int Yatou::teardown()
{
    if (socket_ > 0) {
        if (close(socket_)) {
            std::cerr << "unable to close global socket" << std::endl;
            return 1;
        }
        std::cout << "closed yatou socket" << std::endl;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    try {
        Yatou::setup(INADDR_ANY, 9000);
        Yatou::teardown();
    } catch (Error error) {
        std::cout << error.toString() << std::endl;
    }
}
