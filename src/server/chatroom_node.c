#include "../../include/server/chatroom_node.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct chatroom_node *chatroom_node_init(char *name_chatroom, struct user_node *first_user) {
    struct chatroom_node *new_chatroom_node = malloc(sizeof(struct chatroom_node));

    memset(new_chatroom_node, 0, sizeof(struct chatroom_node));

    snprintf(new_chatroom_node->name, CHATROOM_NAME_LEN, "%s", name_chatroom);
    chatroom_node_add_user_node(new_chatroom_node, first_user);
    first_user->is_in_chatroom = 1;
    new_chatroom_node->num_users = 1;
    new_chatroom_node->next = NULL;

    return new_chatroom_node;
}

void chatroom_node_free(struct chatroom_node *chatroom) {
    free(chatroom);
}

void chatroom_node_add_user_node(struct chatroom_node *chatroom, struct user_node *to_add) {
    to_add->next_in_chatroom = chatroom->user_head;
    chatroom->user_head = to_add;
    chatroom->num_users++;
    to_add->is_in_chatroom = 1;
}

void chatroom_node_remove_user_node(struct chatroom_node *chatroom, struct user_node *to_remove) {

    if (chatroom->user_head == to_remove) {
        chatroom->user_head = to_remove->next_in_chatroom;
        chatroom->num_users--;
        to_remove->is_in_chatroom = 0;

        return;
    }

    for (struct user_node *current = chatroom->user_head; current != NULL; current = current->next_in_chatroom) {

        if (current->next_in_chatroom == to_remove) {
            current->next_in_chatroom = to_remove->next_in_chatroom;
            chatroom->num_users--;
            to_remove->is_in_chatroom = 0;

            return;
        }
    }
}

int chatroom_node_is_user_in(struct chatroom_node *chatroom, struct user_node *user) {
    for (struct user_node *current = chatroom->user_head; current != NULL; current = current->next_in_chatroom) {
        if (current == user) {
            return 1;
        }
    }

    return 0;
}
