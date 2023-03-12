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
        "FILE_ACK"
};

/* structure of a user */
typedef struct user {
    struct socket socket;
    char pseudo[NICK_LEN];
    char date[INFOS_LEN]; /* date of connection */
    int inChatroom; /* 0 if user is not in a chatroom, else 1 */
    struct user *next; /* next user */
} user_t;

/* structure of a chatroom */
typedef struct chatroom {
    char name[NICK_LEN]; /* name of the chatroom */
    user_t *users[10]; /* users who are in the chatroom */
    int nbOfUsers; /* number of users in the chatroom */
} chatroom_t;

struct server {
    struct packet packet;
    chatroom_t *chatrooms[NB_CHATROOMS];
    user_t *users;
    user_t *currentUser;
    struct socket socket;
};

#endif //SERVER_H
