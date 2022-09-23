#ifndef CLIENT_H
#define CLIENT_H

#define NFDS 2

#include "common.h"

struct fileExchange {
    char nickSender[NICK_LEN];
    char nickReceiver[NICK_LEN];
    char fileToSend[NICK_LEN];
    char fileToReceive[NICK_LEN];
};

struct clientP2P {
    struct message msgStruct;
    char payload[MSG_LEN];
    char buffer[MSG_LEN];
    struct socket socket;
    char *userPseudo;
    struct fileExchange fileStruct;
};

struct client {
    struct message msgStruct;
    char payload[MSG_LEN];
    char buffer[MSG_LEN];
    struct socket socket;
    char userPseudo[NICK_LEN];
    int logged;
    struct clientP2P clientP2P;
    struct clientP2P serverP2P;
};

#endif //CLIENT_H
