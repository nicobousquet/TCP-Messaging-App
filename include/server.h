#ifndef SERVER_H
#define SERVER_H

#include "packet.h"
#include "chatroom.h"

static char *msg_type_str[] = {
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

struct Server {
    int socket_fd;
    char ip_addr[16];
    u_short port_num;
    struct Packet packet;
    struct Chatroom *list_chatrooms[NUM_MAX_CHATROOMS];
    int num_chatrooms; /* number of chatrooms not empty */
    struct UserNode *linked_list_users;
    int num_users;
    struct UserNode *current_user;
};

#endif //SERVER_H
