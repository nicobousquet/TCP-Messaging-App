#ifndef CHATROOM_H
#define CHATROOM_H

#include "user_node.h"

struct chatroom_node {
    char name[CHATROOM_NAME_LEN]; /* name of the chatroom_node */
    struct user_node *user_head; /* users who are in the chatroom_node */
    int num_users; /* number of users in the chatroom_node */
    struct chatroom_node *next;
};

struct chatroom_node *chatroom_node_init(char *name_chatroom, struct user_node *first_user);

void chatroom_node_free(struct chatroom_node *chatroom);

void chatroom_node_add_user_node(struct chatroom_node *chatroom, struct user_node *to_add);

void chatroom_node_remove_user_node(struct chatroom_node *chatroom, struct user_node *to_remove);

int chatroom_node_is_user_in(struct chatroom_node *chatroom, struct user_node *user);

#endif //CHATROOM_H
