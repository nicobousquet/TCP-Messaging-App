#include "../../include/server/server.h"
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>

struct server server_init(char *port) {
    struct server server;
    memset(&server, 0, sizeof(struct server));

    struct addrinfo hints, *res, *p;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;

    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }

    for (p = res; p != NULL; p = p->ai_next) {
        server.socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (server.socket_fd == -1) {
            continue;
        }

        if (bind(server.socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(server.socket_fd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        perror("Impossible to bind the socket to an address\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    if ((listen(server.socket_fd, SOMAXCONN)) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    getsockname(server.socket_fd, (struct sockaddr *) &server_addr, &len);
    inet_ntop(AF_INET, &server_addr.sin_addr, server.ip_addr, INET_ADDRSTRLEN);
    server.port_num = ntohs(server_addr.sin_port);

    printf("Server listening on %s:%hu\n", server.ip_addr, server.port_num);

    server.chatroom_head = NULL;
    server.user_head = NULL;

    return server;
}

void server_add_chatroom_node(struct server *server, struct chatroom_node *to_add) {
    to_add->next = server->chatroom_head;
    server->chatroom_head = to_add;
    server->num_chatrooms++;
}

void server_remove_chatroom_node(struct server *server, struct chatroom_node *to_remove) {

    if (server->chatroom_head == to_remove) {
        server->chatroom_head = to_remove->next;
        chatroom_free(to_remove);
        server->num_chatrooms--;

        return;
    }

    for (struct chatroom_node *current = server->chatroom_head; current != NULL; current = current->next) {

        if (current->next == to_remove) {
            current->next = to_remove->next;
            chatroom_free(to_remove);
            server->num_chatrooms--;

            return;
        }
    }
}

void server_add_user_node(struct server *server, struct user_node *to_add) {
    to_add->next_in_server = server->user_head;
    server->user_head = to_add;
    server->num_users++;
}

void server_remove_user_node(struct server *server, struct user_node *to_remove) {

    if (server->user_head == to_remove) {
        server->user_head = to_remove->next_in_server;
        user_node_free(to_remove);
        server->num_users--;

        return;
    }

    for (struct user_node *current = server->user_head; current != NULL; current = current->next_in_server) {

        if (current->next_in_server == to_remove) {
            current->next_in_server = to_remove->next_in_server;
            user_node_free(to_remove);
            server->num_users--;

            return;
        }
    }
}

struct chatroom_node *server_get_chatroom_by_user(struct chatroom_node *head, struct user_node *user) {
    for (struct chatroom_node *current_chatroom = head; current_chatroom != NULL; current_chatroom = current_chatroom->next) {
        for (struct user_node *current_user = current_chatroom->user_head; current_user != NULL; current_user = current_user->next_in_chatroom) {
            if (current_user == user) {
                return current_chatroom;
            }
        }
    }

    return NULL;
}

struct chatroom_node *server_get_chatroom_by_name(struct chatroom_node *head, char *chatroom_name) {
    for (struct chatroom_node *current = head; current != NULL; current = current->next) {
        if (strcmp(current->name, chatroom_name) == 0) {
            return current;
        }
    }

    return NULL;
}

void server_disconnect_user(struct server *server, struct user_node *user_to_disconnect) {
    printf("%s (%s:%hu) on socket %d has disconnected from server\n", user_to_disconnect->nickname, user_to_disconnect->ip_addr, user_to_disconnect->port_num, user_to_disconnect->socket_fd);

    /* if user is in a chatroom_node
       we remove the user from the chatroom_node */
    if (user_to_disconnect->is_in_chatroom == 1) {
        /* getting the chatroom_node in which the user is */
        struct chatroom_node *chatroom = server_get_chatroom_by_user(server->chatroom_head, user_to_disconnect);
        chatroom_node_remove_user_node(chatroom, user_to_disconnect);

        /* informing all the users in the chatroom_node that a user has quit the chatroom_node */
        for (struct user_node *current = chatroom->user_head; current != NULL; current = current->next_in_chatroom) {
            char payload[MSG_LEN];
            snprintf(payload, MSG_LEN, "%s has quit the channel", user_to_disconnect->nickname);

            struct packet res_packet = packet_init("SERVER", DEFAULT, "", payload, strlen(payload));
            packet_send(&res_packet, current->socket_fd);
        }

        /* deleting the chatroom_node if chatroom_node is now empty */
        if (chatroom->num_users == 0) {
            server_remove_chatroom_node(server, chatroom);
        }
    }

    server_remove_user_node(server, user_to_disconnect);
}

/* getting user by its name */
struct user_node *server_get_user_by_name(struct server *server, char *nickname_user) {
    for (struct user_node *current = server->user_head; current != NULL; current = current->next_in_server) {
        if (strcmp(current->nickname, nickname_user) == 0) {
            return current;
        }
    }

    return NULL;
}

void server_handle_nickname_new_req(struct server *server, struct packet *req_packet) {

    for (struct user_node *current = server->user_head; current != NULL; current = current->next_in_server) {
        /* looking if nickname used by another current */
        if (strcmp(current->nickname, req_packet->header.infos) == 0) {
            char payload[] = "Nickname used by another current";
            struct packet res_packet = packet_init("SERVER", DEFAULT, req_packet->header.infos, payload, strlen(payload));
            packet_send(&res_packet, server->current_user->socket_fd);

            return;
        }
    }

    /* if nickname not used yet, adding user nickname */
    snprintf(server->current_user->nickname, NICK_LEN, "%s", req_packet->header.infos);

    if (!server->current_user->is_logged_in) {

        char payload[MSG_LEN];
        snprintf(payload, MSG_LEN, "Welcome on the chat %s, type a command or type /help if you need help", server->current_user->nickname);

        struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
        packet_send(&res_packet, server->current_user->socket_fd);

        server->current_user->is_logged_in = 1;
    } else {
        char payload[MSG_LEN];
        snprintf(payload, MSG_LEN, "You changed your nickname, your nickname is now %s", server->current_user->nickname);

        struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
        packet_send(&res_packet, server->current_user->socket_fd);
    }
}

void server_handle_nickname_list_req(struct server *server, struct packet *req_packet) {

    char payload[MSG_LEN];
    strcpy(payload, "Online users are:\n");

    for (struct user_node *current = server->user_head; current != NULL; current = current->next_in_server) {
        snprintf(payload + strlen(payload), MSG_LEN - strlen(payload), "- %s\n", current->nickname);
    }

    payload[strlen(payload) - 1] = '\0';

    struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
    packet_send(&res_packet, server->current_user->socket_fd);
}

void server_handle_nickname_infos_req(struct server *server, struct packet *req_packet) {
    /* getting user we want information of */
    struct user_node *dest_user = server_get_user_by_name(server, req_packet->header.infos);

    /* if user does not exist */
    if (dest_user == NULL) {
        char payload[] = "Unknown user";
        struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
        packet_send(&res_packet, server->current_user->socket_fd);
    } else { /* if user exists */
        char payload[MSG_LEN];
        snprintf(payload, MSG_LEN, "%s is connected since %s with IP address %s and port number %hu", dest_user->nickname, dest_user->date, dest_user->ip_addr, dest_user->port_num);

        struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
        packet_send(&res_packet, server->current_user->socket_fd);
    }
}

void server_handle_broadcast_send_req(struct server *server, struct packet *req_packet) {

    for (struct user_node *current = server->user_head; current != NULL; current = current->next_in_server) {
        if (strcmp(current->nickname, server->current_user->nickname) == 0) {
            struct packet res_packet = packet_init("me@all", req_packet->header.type, req_packet->header.infos, req_packet->payload, strlen(req_packet->payload));
            packet_send(&res_packet, server->current_user->socket_fd);

        } else {
            char from[NICK_LEN];
            int len = snprintf(from, NICK_LEN, "%s@all", server->current_user->nickname);

            if (len >= NICK_LEN) {
                printf("String truncated\n");
            }

            struct packet res_packet = packet_init(from, req_packet->header.type, req_packet->header.infos, req_packet->payload, strlen(req_packet->payload));
            packet_send(&res_packet, current->socket_fd);
        }
    }
}

void server_handle_unicast_send_req(struct server *server, struct packet *req_packet) {
    struct user_node *dest_user = server_get_user_by_name(server, req_packet->header.infos);

    /* if dest user exists */
    if (dest_user != NULL) {

        if (dest_user == server->current_user) {
            char payload[] = "You cannot send a message to yourself";
            struct packet res_packet = packet_init("SERVER", DEFAULT, req_packet->header.infos, payload, strlen(payload));
            packet_send(&res_packet, dest_user->socket_fd);

        } else {
            struct packet res_packet = packet_init(req_packet->header.from, req_packet->header.type, req_packet->header.infos, req_packet->payload, strlen(req_packet->payload));
            packet_send(&res_packet, dest_user->socket_fd);

            char from[NICK_LEN];
            int len = snprintf(from, NICK_LEN, "me@%s", dest_user->nickname);

            if (len >= NICK_LEN) {
                printf("String truncated\n");
            }

            packet_set(&res_packet, from, req_packet->header.type, req_packet->header.infos, req_packet->payload, strlen(req_packet->payload));
            packet_send(&res_packet, server->current_user->socket_fd);

        }
    } else {
        /* if dest user does not exist */
        char payload[MSG_LEN];
        snprintf(payload, MSG_LEN, "User %s does not exist", req_packet->header.infos);

        struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
        packet_send(&res_packet, server->current_user->socket_fd);
    }
}

void server_handle_multicast_create_req(struct server *server, struct packet *req_packet) {
    /* we check if chatroom_node name is not used yet */
    for (struct chatroom_node *current = server->chatroom_head; current != NULL; current = current->next) {
        if (strcmp(current->name, req_packet->header.infos) == 0) {

            char payload[] = "Chatroom name used by other users, please choose a new chatroom name";

            struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
            packet_send(&res_packet, server->current_user->socket_fd);

            return;
        }
    }

    /* if chatrooom's name does not exist yet */
    /* we remove the user from all the chatrooms he is in before creating a new chatroom_node */
    struct chatroom_node *chatroom = server_get_chatroom_by_user(server->chatroom_head, server->current_user);

    if (chatroom != NULL) {
        chatroom_node_remove_user_node(chatroom, server->current_user);

        char payload[MSG_LEN];
        snprintf(payload, MSG_LEN, "You have quit the channel %s", chatroom->name);

        struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
        packet_send(&res_packet, server->current_user->socket_fd);

        for (struct user_node *current = chatroom->user_head; current != NULL; current = current->next_in_chatroom) {
            snprintf(payload, MSG_LEN, "%s has quit the channel", server->current_user->nickname);

            char from[NICK_LEN];
            int len = snprintf(from, NICK_LEN, "SERVER@%s", chatroom->name);

            if (len >= NICK_LEN) {
                printf("String truncated\n");
            }

            packet_set(&res_packet, from, req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
            packet_send(&res_packet, current->socket_fd);
        }

        /* if chatroom_node is empty, destroying it */
        if (chatroom->num_users == 0) {
            snprintf(payload, MSG_LEN, "You were the last user in %s, this channel has been destroyed", chatroom->name);

            packet_set(&res_packet, "SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
            packet_send(&res_packet, server->current_user->socket_fd);

            server_remove_chatroom_node(server, chatroom);
        }
    }

    /* creating a new chatroom_node */
    struct chatroom_node *new_chatroom = chatroom_init(req_packet->header.infos, server->current_user);
    server_add_chatroom_node(server, new_chatroom);

    char payload[MSG_LEN];
    snprintf(payload, MSG_LEN, "You have created channel %s", new_chatroom->name);

    struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
    packet_send(&res_packet, server->current_user->socket_fd);
}

void server_handle_multicast_list_req(struct server *server, struct packet *req_packet) {
    if (server->num_chatrooms != 0) {

        char payload[MSG_LEN];
        strcpy(payload, "Online chatrooms are:\n");

        for (struct chatroom_node *current = server->chatroom_head; current != NULL; current = current->next) {
            snprintf(payload + strlen(payload), MSG_LEN - strlen(payload), "- %s\n", current->name);
        }

        payload[strlen(payload) - 1] = '\0';

        struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
        packet_send(&res_packet, server->current_user->socket_fd);

        return;
    }

    char payload[MSG_LEN];
    strcpy(payload, "There is no chatrooms created\n");

    struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
    packet_send(&res_packet, server->current_user->socket_fd);
}

void server_handle_multicast_join_req(struct server *server, struct packet *req_packet) {
    if (server->num_chatrooms != 0) {
        /* getting chatroom_node user wants to join */
        struct chatroom_node *chatroom_to_join = server_get_chatroom_by_name(server->chatroom_head, req_packet->header.infos);

        /* if chatroom_node exists */
        if (chatroom_to_join != NULL) {
            struct chatroom_node *current_chatroom = server_get_chatroom_by_user(server->chatroom_head, server->current_user);

            /* if user is in a chatroom_node*/
            if (current_chatroom != NULL) {
                /* if user is yet in the chatroom_node */
                if (current_chatroom == chatroom_to_join) {

                    char payload[MSG_LEN];
                    strcpy(payload, "You were yet in this chatroom");

                    struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                    packet_send(&res_packet, server->current_user->socket_fd);

                    return;

                } else {

                    chatroom_node_remove_user_node(current_chatroom, server->current_user);

                    char payload[MSG_LEN];
                    snprintf(payload, MSG_LEN, "You have quit the channel %s", current_chatroom->name);

                    struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                    packet_send(&res_packet, server->current_user->socket_fd);

                    /* if chatroom_node is empty, destroying it */
                    if (current_chatroom->num_users == 0) {
                        snprintf(payload, MSG_LEN, "You were the last user in %s, this channel has been destroyed", current_chatroom->name);

                        packet_set(&res_packet, "SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                        packet_send(&res_packet, server->current_user->socket_fd);

                        server_remove_chatroom_node(server, current_chatroom);
                    }

                    for (struct user_node *current_user = current_chatroom->user_head; current_user != NULL; current_user = current_user->next_in_chatroom) {
                        snprintf(payload, MSG_LEN, "%s has quit the channel", server->current_user->nickname);

                        char from[NICK_LEN];
                        int len = snprintf(from, NICK_LEN, "SERVER@%s", current_chatroom->name);

                        if (len >= NICK_LEN) {
                            printf("String truncated\n");
                        }

                        res_packet = packet_init(from, req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                        packet_send(&res_packet, current_user->socket_fd);
                    }
                }
            }

            /* joining chatroom_node */
            chatroom_node_add_user_node(chatroom_to_join, server->current_user);

            char payload[MSG_LEN];
            snprintf(payload, MSG_LEN, "You have joined %s", req_packet->header.infos);

            struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
            packet_send(&res_packet, server->current_user->socket_fd);

            for (struct user_node *current = chatroom_to_join->user_head; current != NULL; current = current->next_in_chatroom) {
                /* informing the other users in the chatroom_node that a new user joined*/
                snprintf(payload, MSG_LEN, "%s has joined the channel", server->current_user->nickname);

                char from[NICK_LEN];
                int len = snprintf(from, NICK_LEN, "SERVER@%s", chatroom_to_join->name);

                if (len >= NICK_LEN) {
                    printf("String truncated\n");
                }
                res_packet = packet_init(from, req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                packet_send(&res_packet, current->socket_fd);
            }

            return;
        }
    }

    /* if chatroom_node does not exist */
    char payload[MSG_LEN];
    snprintf(payload, MSG_LEN, "Channel %s does not exist", req_packet->header.infos);

    struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
    packet_send(&res_packet, server->current_user->socket_fd);
}

void server_handle_multicast_quit_req(struct server *server, struct packet *req_packet) {
    /* getting chatroom_node user wants to quit */
    if (server->num_chatrooms != 0) {
        struct chatroom_node *chatroom = server_get_chatroom_by_name(server->chatroom_head, req_packet->header.infos);

        if (chatroom != NULL) {

            if (chatroom_is_user_in(chatroom, server->current_user)) {

                chatroom_node_remove_user_node(chatroom, server->current_user);

                for (struct user_node *current = chatroom->user_head; current != NULL; current = current->next_in_chatroom) {
                    char payload[MSG_LEN];
                    snprintf(payload, MSG_LEN, "%s has quit the channel", server->current_user->nickname);

                    char from[NICK_LEN];
                    int len = snprintf(from, NICK_LEN, "SERVER@%s", chatroom->name);

                    if (len >= NICK_LEN) {
                        printf("String truncated\n");
                    }

                    struct packet res_packet = packet_init(from, req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                    packet_send(&res_packet, current->socket_fd);
                }

                char payload[MSG_LEN];
                snprintf(payload, MSG_LEN, "You have quit the channel %s", chatroom->name);

                struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                packet_send(&res_packet, server->current_user->socket_fd);

                /* if chatroom_node is now empty */
                if (chatroom->num_users == 0) {
                    snprintf(payload, MSG_LEN, "You were the last user in this channel, %s has been destroyed", chatroom->name);

                    packet_set(&res_packet, "SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                    packet_send(&res_packet, server->current_user->socket_fd);

                    server_remove_chatroom_node(server, chatroom);
                }
            } else {
                /* if user was not in the chatroom_node */
                char payload[MSG_LEN];
                snprintf(payload, MSG_LEN, "You were not in chatroom_node %s", chatroom->name);

                struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                packet_send(&res_packet, server->current_user->socket_fd);
            }

            return;
        }
    }

    /* if chatroom_node does not exist */
    char payload[MSG_LEN];
    snprintf(payload, MSG_LEN, "Channel %s does not exist", req_packet->header.infos);

    struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
    packet_send(&res_packet, server->current_user->socket_fd);
}

void server_handle_multicast_send_req(struct server *server, struct packet *req_packet) {
    /* if user is not in a chatroom_node*/
    if (server->current_user->is_in_chatroom == 0) {

        struct packet res_packet = packet_init("SERVER", ECHO_SEND, req_packet->header.infos, req_packet->payload, strlen(req_packet->payload));
        packet_send(&res_packet, server->current_user->socket_fd);

    } else {
        /* getting the chatroom_node the user wants to send the message in */
        struct chatroom_node *chatroom = server_get_chatroom_by_user(server->chatroom_head, server->current_user);

        /* sending message to all the users in the chatroom_node */
        for (struct user_node *current = chatroom->user_head; current != NULL; current = current->next_in_chatroom) {
            if (strcmp(current->nickname, server->current_user->nickname) == 0) {

                char from[NICK_LEN];
                int len = snprintf(from, NICK_LEN, "me@%s", chatroom->name);

                if (len >= NICK_LEN) {
                    printf("String truncated\n");
                }

                struct packet res_packet = packet_init(from, req_packet->header.type, req_packet->header.infos, req_packet->payload, strlen(req_packet->payload));
                packet_send(&res_packet, current->socket_fd);
            } else {

                char from[NICK_LEN];
                int len = snprintf(from, NICK_LEN, "%s@%s", server->current_user->nickname, chatroom->name);

                if (len >= NICK_LEN) {
                    printf("String truncated\n");
                }

                struct packet res_packet = packet_init(from, req_packet->header.type, req_packet->header.infos, req_packet->payload, strlen(req_packet->payload));
                packet_send(&res_packet, current->socket_fd);
            }
        }
    }
}

void server_handle_file_req(struct server *server, struct packet *req_packet) {
    /* getting the user he wants to send file to */
    struct user_node *dest_user = server_get_user_by_name(server, req_packet->header.infos);

    /* if user does not exist */
    if (dest_user == NULL) {

        char payload[MSG_LEN];
        snprintf(payload, MSG_LEN, "%s does not exist", req_packet->header.infos);

        struct packet res_packet = packet_init("SERVER", DEFAULT, req_packet->header.infos, payload, strlen(payload));
        packet_send(&res_packet, server->current_user->socket_fd);

    } else if (dest_user == server->current_user) {

        char payload[MSG_LEN];
        strcpy(payload, "You cannot send a file to yourself");

        struct packet res_packet = packet_init("SERVER", DEFAULT, req_packet->header.infos, payload, strlen(payload));
        packet_send(&res_packet, server->current_user->socket_fd);

    } else {

        struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.from, req_packet->payload, strlen(req_packet->payload));
        packet_send(&res_packet, dest_user->socket_fd);
    }
}

void server_handle_file_accept_res(struct server *server, struct packet *req_packet) {
    struct user_node *dest_user = server_get_user_by_name(server, req_packet->header.infos);

    struct packet res_packet = packet_init(req_packet->header.from, req_packet->header.type, req_packet->header.infos, req_packet->payload, strlen(req_packet->payload));
    packet_send(&res_packet, dest_user->socket_fd);
}

void server_handle_file_reject_req(struct server *server, struct packet *req_packet) {
    struct user_node *dest_user = server_get_user_by_name(server, req_packet->header.infos);

    char payload[MSG_LEN];
    snprintf(payload, MSG_LEN, "%s cancelled file transfer", server->current_user->nickname);

    struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
    packet_send(&res_packet, dest_user->socket_fd);
}