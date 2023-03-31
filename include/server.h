#ifndef SERVER_H
#define SERVER_H

#define NB_USERS 10 //nombre de clients possibles sur le serveur
#define NB_CHATROOMS 10 //nombre de salons

#include "common.h"

static char *msgTypeStr[] = {
        "DEFAULT",
        "NICKNAME_NEW",
        "NICKNAME_LIST",
        "NICKNAME_INFOS",
        "ECHO_SEND",
        "UNICAST_SEND",
        "BROADCAST_SEND",
        "MULTICAST_CREATE",
        "MULTICAST_LIST",
        "MULTICAST_JOIN",
        "MULTICAST_SEND",
        "MULTICAST_QUIT",
        "FILE_REQUEST",
        "FILE_ACCEPT",
        "FILE_REJECT",
        "FILE_SEND",
        "FILE_ACK",
        "FILENAME",
        "HELP",
        "QUIT"
};

/* structure of a user */
struct user {
    int socketFd;
    char ipAddr[16];
    u_short portNum;
    char pseudo[NICK_LEN];
    char date[INFOS_LEN]; /* date of connection */
    int inChatroom; /* 0 if user is not in a chatroom, else 1 */
    struct user *next; /* next user */
};

/* structure of a chatroom */
struct chatroom {
    char name[NICK_LEN]; /* name of the chatroom */
    struct user *users[10]; /* users who are in the chatroom */
    int numOfUsers; /* number of users in the chatroom */
};

struct server {
    int socketFd;
    char ipAddr[16];
    u_short portNum;
    struct packet packet;
    struct chatroom *chatrooms[NB_CHATROOMS];
    struct user *users;
    struct user *currentUser;
};

#endif //SERVER_H
