#include <algorithm>
#include <arpa/inet.h>
#include <cstddef>
#include <fcntl.h>
#include <iostream>
#include <limits>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "yatou.h"

#define DEBUG true
#define DEFAULT_MAX_CONNECTIONS 1024

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned long ip4_addr_t;


/*
 * set_if_ip(): Sets the ip for the interface in *ifr and sets the
 *              subnet mask if provided
 */
void Yatou::set_if_ip(struct ifreq *ifr,
                      int sockfd,
                      const char* inet_address,
                      const char* subnet)
{
    struct sockaddr_in inet_addr, subnet_mask;

    /// prepare address
    inet_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, inet_address, &(inet_addr.sin_addr)) != 1) {
        throw Yatou::Error("Invalid inet_address");
    }

    /// prepare subnet mask
    subnet_mask.sin_family = AF_INET;
    if (inet_pton(AF_INET, subnet, &(subnet_mask.sin_addr)) != 1) {
        throw Yatou::Error("Invalid subnet mask");
    }

    /// put addr in ifr structure
    memcpy(&(ifr->ifr_addr), &inet_addr, sizeof (struct sockaddr));
    if (ioctl(sockfd, SIOCSIFADDR, ifr) < 0) {
        throw Yatou::Error("Unable to set IP address");
    }

    /// put mask in ifr structure
    if (subnet) {
        memcpy(&(ifr->ifr_addr), &subnet_mask, sizeof (struct sockaddr));
        if(ioctl(sockfd, SIOCSIFNETMASK, ifr) < 0) {
            throw Yatou::Error("Unable to set subnet mask");
        }
    }
}

/*
 * set_if_up(): Marks the interface "up"
 */
void Yatou::set_if_up(struct ifreq *ifr, int sockfd)
{
    ifr->ifr_flags |= IFF_UP;
    ifr->ifr_flags |= IFF_RUNNING;

    if (ioctl(sockfd, SIOCSIFFLAGS, ifr) < 0)  {
        throw Yatou::Error("SIOCSIFFLAGS");
    }
}


/*
 *  Yatou::create_tun() - Creates a new TUN device
 */
struct ifreq Yatou::create_tun(std::string device_name)
{
    char tun_device_name[IFNAMSIZ];
    const char *clonedev = "/dev/net/tun";
    struct ifreq ifr;

    // open the clone device
    yatou_fd_ = open(clonedev, O_RDWR);
    if (yatou_fd_ < 0) {
        throw Error("Unable to open clone device");
    }

    // prepare ifr struct
    memset(&ifr, 0, sizeof(ifr));

    // set the flags to create a TUN device
    ifr.ifr_flags = IFF_TUN;

    // prepare the name for the TUN device
    strcpy(tun_device_name, device_name.c_str());
    strncpy(ifr.ifr_name, tun_device_name, IFNAMSIZ);

    // create the TUN device
    if (ioctl(yatou_fd_, TUNSETIFF, (void *) &ifr) < 0) {
        throw Error(std::string("Unable to create TUN device"));
    }

    // remove persistent status
    if(ioctl(yatou_fd_, TUNSETPERSIST, 0) < 0){
        throw Error(std::string("Failed to disable TUN persist"));
    }

    return ifr;
}

/*
 *  Yatou::setup_tun() - Creates TUN device
 *
 *  returns: file descriptor to TUN clone
 */
void Yatou::setup()
{
    // Setup has already succeeded, don't retry
    if (yatou_fd_) { return; }

    struct ifreq ifr = create_tun("yatou");

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        throw Yatou::Error("Cannot open udp socket");
    }

    set_if_ip(&ifr, sock, "10.0.0.1", "255.255.255.0");
    set_if_up(&ifr, sock);
}


void print_hex(char* buffer, size_t len)
{
    int width = 0;
    for (size_t i = 0; i < len; i++, width++) {
        printf("%0x ", (unsigned char)buffer[i]);
        if (width >= 40) {
            printf("\n");
            width = 0;
        }
    }
    printf("\n");
}

void printn(char* buffer, size_t len)
{
    int width = 0;
    for (size_t i = 0; i < len; i++, width++) {
        printf("%c", (unsigned char)buffer[i]);
        if (width >= 40) {
            printf("\n");
            width = 0;
        }
    }
    printf("\n");
}


void Yatou::teardown()
{
    if (yatou_fd_) {
        close(yatou_fd_);
    }
}

void startServer()
{
    Yatou::setup();
    int nread;
    char buffer[2048];

    int count = 0;

    std::cout << "reading from fd" << std::endl;
    while(1) {
        nread = read(Yatou::yatou_fd_, buffer, sizeof(buffer));

        if(nread < 0) {
            perror("Reading from interface");
            close(Yatou::yatou_fd_);
            exit(1);
        }

        printf("%d: Read %d bytes from device\n", count, nread);
        print_hex(buffer, nread);

        if (count++ > 10) {
            break;
        }
    }

    Yatou::teardown();
}


int main()
{
    try {
        startServer();
    } catch (Yatou::Error error) {
        std::cout << error.toString() << std::endl;
        Yatou::teardown();
    }
    Yatou::teardown();
}
