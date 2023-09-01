#include "../../include/server/chatroom.h"
#include <string.h>
#include <stdlib.h>

struct chatroom *chatroom_init(char *name_chatroom, struct user_node *first_user) {
    struct chatroom *new_chatroom = malloc(sizeof(struct chatroom));
    memset(new_chatroom, 0, sizeof(struct chatroom));
    strcpy(new_chatroom->name, name_chatroom);
    new_chatroom->list_users[0] = first_user;
    first_user->is_in_chatroom = 1;
    new_chatroom->num_users = 1;

    return new_chatroom;
}

void chatroom_free(struct chatroom *chatroom) {
    free(chatroom);
}

struct chatroom **chatroom_get_by_user(struct chatroom **list_chatrooms, struct user_node *user) {
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

struct chatroom **chatroom_get_by_name(struct chatroom **list_chatrooms, char *chatroom_name) {
    for (int j = 0; j < NUM_MAX_CHATROOMS; j++) {
        if (list_chatrooms[j] != NULL) {
            if (strcmp(list_chatrooms[j]->name, chatroom_name) == 0) {
                return &(list_chatrooms[j]);
            }
        }
    }

    return NULL;
}

struct user_node **chatroom_remove_user(struct chatroom *chatroom, struct user_node *user) {
    for (int k = 0; k < NUM_MAX_USERS; k++) {
        if (chatroom->list_users[k] != NULL && chatroom->list_users[k] == user) {
            chatroom->list_users[k] = NULL;
            chatroom->num_users--;

            return &(chatroom->list_users[k]);
        }
    }

    return NULL;
}
