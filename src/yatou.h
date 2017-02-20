#ifndef _YATOU_H
#define _YATOU_H

namespace Yatou {
    typedef uint16_t port_t;
    typedef uint32_t socket_id_t;

    const socket_id_t MAX_SOCKET_T = std::numeric_limits<socket_id_t>::max();

    /* TUN file descriptor, this is the special file descriptor that
     * the caller will use to talk
     */
    int yatou_fd_ = 0;

    class Error {
    private:
        std::string message_;
    public:
        Error(std::string message) : message_(message) { }
        std::string toString() { return message_; }
    };

    struct ifreq create_tun(std::string device_name);

    void set_if_ip(struct ifreq *ifr,
                   int sockfd,
                   const char* inet_address,
                   const char* subnet);

    void Yatou::set_if_up(struct ifreq *ifr, int sockfd);

    void setup();
    void teardown();
}

#endif // _YATOU_H
