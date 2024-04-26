#include "../../include/server/chatroom_node.h"
#include <string.h>
#include <stdlib.h>

void chatroom_node_add_user_node(struct chatroom_node *chatroom, struct user_node *to_add) {
    to_add->next_in_chatroom = chatroom->user_head;
    chatroom->user_head = to_add;
}

void chatroom_node_remove_user_node(struct chatroom_node *chatroom, struct user_node *to_remove) {

    if (chatroom->user_head == to_remove) {
        chatroom->user_head = to_remove->next_in_chatroom;
        chatroom->num_users--;

        return;
    }

    for (struct user_node *current = chatroom->user_head; current != NULL; current = current->next_in_chatroom) {

        if (current->next_in_chatroom == to_remove) {
            current->next_in_chatroom = to_remove->next_in_chatroom;
            chatroom->num_users--;

            return;
        }
    }
}

struct chatroom_node *chatroom_init(char *name_chatroom, struct user_node *first_user) {
    struct chatroom_node *new_chatroom_node = malloc(sizeof(struct chatroom_node));

    memset(new_chatroom_node, 0, sizeof(struct chatroom_node));

    strcpy(new_chatroom_node->name, name_chatroom);
    chatroom_node_add_user_node(new_chatroom_node, first_user);
    first_user->is_in_chatroom = 1;
    new_chatroom_node->num_users = 1;
    new_chatroom_node->next = NULL;

    return new_chatroom_node;
}

void chatroom_free(struct chatroom_node *chatroom) {
    free(chatroom);
}

int chatroom_is_user_in(struct chatroom_node *chatroom, struct user_node *user) {
    for (struct user_node *current = chatroom->user_head; current != NULL; current = current->next_in_chatroom) {
        if (current == user) {
            return 1;
        }
    }

    return 0;
}
