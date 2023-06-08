#ifndef CHATROOM_H
#define CHATROOM_H

#include "user_node.h"

struct chatroom {
    char name[NICK_LEN]; /* name of the chatroom */
    struct userNode *list_users[NUM_MAX_USERS]; /* users who are in the chatroom */
    int num_users; /* number of users in the chatroom */
};

struct chatroom *chatroom_init(char *name_chatroom, struct userNode *first_user);

struct chatroom **chatroom_get_by_user(struct chatroom **list_chatrooms, struct userNode *user);

struct chatroom **chatroom_get_by_name(struct chatroom **list_chatrooms, char *chatroom_name);

struct userNode **chatroom_remove_user(struct chatroom *chatroom, struct userNode *user);

#endif //CHATROOM_H
