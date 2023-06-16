#include "../../include/server/user_node.h"
#include <stdlib.h>
#include <string.h>

struct user_node *user_node_init(int socket_fd, char *ip_addr, u_short port_num, char *nickname, char *date) {
    struct user_node *user = malloc(sizeof(struct user_node));
    user->socket_fd = socket_fd;
    strcpy(user->ip_addr, ip_addr);
    user->port_num = port_num;
    strcpy(user->nickname, nickname);
    strcpy(user->date, date);
    user->is_in_chatroom = 0;
    user->is_logged_in = 0;
    user->next = NULL;
    return user;
}

/* adding user in the list */
void user_node_add(struct user_node **linked_list_users, struct user_node *new_user_node) {
    new_user_node->next = *linked_list_users;
    *linked_list_users = new_user_node;
}

/* freeing user node in the list */
void user_node_remove(struct user_node **linked_list_users, struct user_node *user_to_remove) {
    struct user_node *newNode = malloc(sizeof(struct user_node));
    newNode->next = *linked_list_users;
    *linked_list_users = newNode;
    struct user_node *current;
    /* looking for the user we want to free */
    for (current = *linked_list_users; current->next != NULL; current = current->next) {
        if (current->next == user_to_remove) {
            break;
        }
    }
    /* freeing user */
    struct user_node *tmp = current->next;
    current->next = (current->next)->next;
    free(tmp);
    /* freeing fake first node of the list */
    tmp = *linked_list_users;
    *linked_list_users = (*linked_list_users)->next;
    free(tmp);
}
