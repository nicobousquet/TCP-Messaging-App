#ifndef USER_NODE_H
#define USER_NODE_H

#include "../constants.h"
#include <sys/types.h>
#include <arpa/inet.h>

/* structure of a user */
struct user_node {
    int socket_fd;
    char ip_addr[INET_ADDRSTRLEN];
    u_short port_num;
    char nickname[NICK_LEN];
    int is_logged_in;
    char date[INFOS_LEN]; /* date of connection */
    int is_in_chatroom; /* 0 if user is not in a chatroom_node, else 1 */
    struct user_node *next_in_server; /* next_in_server user */
    struct user_node *next_in_chatroom;
};

struct user_node *user_node_init(int socket_fd, char *ip_addr, u_short port_num, char *nickname, char *date);

void user_node_free(struct user_node *user_node);

#endif //USER_NODE_H
