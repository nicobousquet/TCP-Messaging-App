#ifndef CLIENT_H
#define CLIENT_H

#define NFDS 2

#include "common.h"
#include <poll.h>

struct client {
    int socketFd;
    char ipAddr[16];
    u_short portNum;
    struct packet packet;
    char buffer[MSG_LEN];
    char userPseudo[NICK_LEN];
    char fileToSend[NICK_LEN];
    char fileToReceive[NICK_LEN];
    int loggedIn;
};

#endif //CLIENT_H
