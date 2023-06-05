#include "../include/chatroom.h"
#include <string.h>
#include <stdlib.h>

struct Chatroom *chatroom_init(char *name_chatroom, struct UserNode *first_user) {
    struct Chatroom *new_chatroom = malloc(sizeof(struct Chatroom));
    memset(new_chatroom, 0, sizeof(struct Chatroom));
    strcpy(new_chatroom->name, name_chatroom);
    new_chatroom->list_users[0] = first_user;
    first_user->is_in_chatroom = 1;
    new_chatroom->num_users = 1;
    return new_chatroom;
}

struct Chatroom **chatroom_get_by_user(struct Chatroom **list_chatrooms, struct UserNode *user) {
    for (int j = 0; j < NUM_MAX_CHATROOMS; j++) {
        if (list_chatrooms[j] != NULL) {
            for (int k = 0; k < NUM_MAX_USERS; k++) {
                if (list_chatrooms[j]->list_users[k] != NULL) {
                    if (list_chatrooms[j]->list_users[k] == user) {
                        return &(list_chatrooms[j]);
                    }
                }
            }
        }
    }
    return NULL;
}

struct Chatroom **chatroom_get_by_name(struct Chatroom **list_chatrooms, char *chatroom_name) {
    for (int j = 0; j < NUM_MAX_CHATROOMS; j++) {
        if (list_chatrooms[j] != NULL) {
            if (strcmp(list_chatrooms[j]->name, chatroom_name) == 0) {
                return &(list_chatrooms[j]);
            }
        }
    }
    return NULL;
}

struct UserNode **chatroom_remove_user(struct Chatroom *chatroom, struct UserNode *user) {
    for (int k = 0; k < NUM_MAX_USERS; k++) {
        if (chatroom->list_users[k] != NULL && chatroom->list_users[k] == user) {
            chatroom->list_users[k] = NULL;
            chatroom->num_users--;
            return &(chatroom->list_users[k]);
        }
    }
    return NULL;
}
