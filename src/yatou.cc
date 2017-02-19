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
 * set_ip(): Sets the ip for the interface in *ifr and sets the subnet
 *           mask if provided
 */
void set_ip(struct ifreq *ifr,
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
 *  Yatou::setup_tun() - Creates TUN device
 *
 *  returns: file descriptor to TUN clone
 */
void Yatou::setup()
{
    // Setup has already succeeded, don't retry
    if (yatou_fd_) { return; }

    // location of the clone device
    const char *clonedev = "/dev/net/tun";
    // interface request structure
    struct ifreq ifr;
    // name of our TUN device
    char device[IFNAMSIZ];
    strcpy(device, "yatou");

    int err;
    int flags = IFF_TUN;

    /* open the clone device */
    if( (yatou_fd_ = open(clonedev, O_RDWR)) < 0 ) {
        throw Error(std::string("Unable to open clone device"));
    }

    std::cout << "opened clone device: " << yatou_fd_ << std::endl;

    /* preparation of the struct ifr, of type "struct ifreq" */
    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = flags;   /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

    if (*device) {
        /* if a device name was specified, put it in the structure; otherwise,
         * the kernel will try to allocate the "next" device of the
         * specified type */
        strncpy(ifr.ifr_name, device, IFNAMSIZ);
    }

    /* try to create the device */
    if( (err = ioctl(yatou_fd_, TUNSETIFF, (void *) &ifr)) < 0 ) {
        std::cout << "failed" << std::endl;
        close(yatou_fd_);
        throw Error(std::string("Unable to create TUN device"));
    }

    /* if the operation was successful, write back the name of the
     * interface to the variable "dev", so the caller can know
     * it. Note that the caller MUST reserve space in *dev (see calling
     * code below) */
    strcpy((char*)device, ifr.ifr_name);

    std::cout << "removing persistent status" << std::endl;

    /* remove persistent status */
    if(ioctl(yatou_fd_, TUNSETPERSIST, 0) < 0){
        perror("disabling TUNSETPERSIST");
        exit(1);
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        throw Yatou::Error("Cannot open udp socket");
    }

    set_ip2(&ifr, sock, "10.0.0.1", "255.255.255.0");

    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        throw Yatou::Error("SIOCGIFFLAGS");
    }

    ifr.ifr_flags |= IFF_UP;
    ifr.ifr_flags |= IFF_RUNNING;

    if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0)  {
        throw Yatou::Error("SIOCSIFFLAGS");
    }
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
    }
}
