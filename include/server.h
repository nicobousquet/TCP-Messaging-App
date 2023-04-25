#ifndef SERVER_H
#define SERVER_H

#define NUM_MAX_USERS 10 /* maximum possible number of users connected to the server */
#define NUM_MAX_CHATROOMS 10 /* maximum possible number of chatrooms in the server */

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
    char nickname[NICK_LEN];
    int loggedIn;
    char date[INFOS_LEN]; /* date of connection */
    int inChatroom; /* 0 if user is not in a chatroom, else 1 */
    struct user *next; /* next user */
};

/* structure of a chatroom */
struct chatroom {
    char name[NICK_LEN]; /* name of the chatroom */
    struct user *usersList[NUM_MAX_USERS]; /* users who are in the chatroom */
    int numUsersInChatroom; /* number of users in the chatroom */
};

struct server {
    int socketFd;
    char ipAddr[16];
    u_short portNum;
    struct packet packet;
    struct chatroom *chatroomsList[NUM_MAX_CHATROOMS];
    int numChatrooms; /* number of chatrooms not empty */
    struct user *usersLinkedList;
    int numUsers;
    struct user *currentUser;
};

#endif //SERVER_H
