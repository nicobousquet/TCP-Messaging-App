#ifndef CLIENT_H
#define CLIENT_H

#define NUM_FDS 2

#include "packet.h"
#include <sys/types.h>
#include <poll.h>

struct Client {
    int fd_socket;
    char ip_addr[16];
    u_short port_num;
    struct Packet packet;
    char buffer[MSG_LEN];
    char nickname[NICK_LEN];
    char file_to_send[NICK_LEN];
    char file_to_receive[NICK_LEN];
};

#endif //CLIENT_H
