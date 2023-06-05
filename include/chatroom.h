#ifndef CHATROOM_H
#define CHATROOM_H

#include "user_node.h"

struct Chatroom {
    char name[NICK_LEN]; /* name of the chatroom */
    struct UserNode *list_users[NUM_MAX_USERS]; /* users who are in the chatroom */
    int num_users; /* number of users in the chatroom */
};

struct Chatroom *chatroom_init(char *name_chatroom, struct UserNode *first_user);

struct Chatroom **chatroom_get_by_user(struct Chatroom **list_chatrooms, struct UserNode *user);

struct Chatroom **chatroom_get_by_name(struct Chatroom **list_chatrooms, char *chatroom_name);

struct UserNode **chatroom_remove_user(struct Chatroom *chatroom, struct UserNode *user);

#endif //CHATROOM_H
