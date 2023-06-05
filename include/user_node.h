#ifndef USER_NODE_H
#define USER_NODE_H

#include "../include/constants.h"
#include <sys/types.h>

/* structure of a user */
struct UserNode {
    int socket_fd;
    char ip_addr[16];
    u_short port_num;
    char nickname[NICK_LEN];
    int is_logged_in;
    char date[INFOS_LEN]; /* date of connection */
    int is_in_chatroom; /* 0 if user is not in a chatroom, else 1 */
    struct UserNode *next; /* next user */
};

struct UserNode *user_node_init(int socket_fd, char *ip_addr, u_short port_num, char *nickname, char *date);

void user_node_add(struct UserNode **linked_list_users, struct UserNode *new_user_node);

void user_node_remove(struct UserNode **linked_list_users, struct UserNode *user_to_remove);

#endif //USER_NODE_H
