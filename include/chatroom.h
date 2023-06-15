#ifndef CHATROOM_H
#define CHATROOM_H

#include "user_node.h"

struct chatroom {
    char name[NICK_LEN]; /* name of the chatroom */
    struct user_node *list_users[NUM_MAX_USERS]; /* users who are in the chatroom */
    int num_users; /* number of users in the chatroom */
};

struct chatroom *chatroom_init(char *name_chatroom, struct user_node *first_user);

struct chatroom **chatroom_get_by_user(struct chatroom **list_chatrooms, struct user_node *user);

struct chatroom **chatroom_get_by_name(struct chatroom **list_chatrooms, char *chatroom_name);

struct user_node **chatroom_remove_user(struct chatroom *chatroom, struct user_node *user);

#endif //CHATROOM_H
