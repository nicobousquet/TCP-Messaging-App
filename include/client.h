#ifndef CLIENT_H
#define CLIENT_H

#define NUM_FDS 2

#include "common.h"
#include <poll.h>

struct client {
    int socketFd;
    char ipAddr[16];
    u_short portNum;
    struct packet packet;
    char buffer[MSG_LEN];
    char userNickname[NICK_LEN];
    char fileToSend[NICK_LEN];
    char fileToReceive[NICK_LEN];
};

#endif //CLIENT_H
