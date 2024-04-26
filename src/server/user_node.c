#include "../../include/server/user_node.h"
#include <stdlib.h>
#include <string.h>

struct user_node *user_node_init(int socket_fd, char *ip_addr, u_short port_num, char *nickname, char *date) {
    struct user_node *user = malloc(sizeof(struct user_node));

    memset(user, 0, sizeof(struct user_node));

    user->socket_fd = socket_fd;
    strcpy(user->ip_addr, ip_addr);
    user->port_num = port_num;
    strcpy(user->nickname, nickname);
    strcpy(user->date, date);
    user->is_in_chatroom = 0;
    user->is_logged_in = 0;
    user->next_in_server = NULL;
    user->next_in_chatroom = NULL;

    return user;
}

void user_node_free(struct user_node *user_node) {
    free(user_node);
}
