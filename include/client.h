#ifndef CLIENT_H
#define CLIENT_H

#define NFDS 2

#include "common.h"
#include <poll.h>

struct fileExchange {
    char nickSender[NICK_LEN];
    char nickReceiver[NICK_LEN];
    char fileToSend[NICK_LEN];
    char fileToReceive[NICK_LEN];
};

struct client {
    struct message msgStruct;
    char payload[MSG_LEN];
    char buffer[MSG_LEN];
    struct socket socket;
    char userPseudo[NICK_LEN];
    char fileToSend[NICK_LEN];
    int loggedIn;
};

#endif //CLIENT_H
