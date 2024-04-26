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

    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &result) != 0) {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        server.socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (server.socket_fd == -1) {
            continue;
        }

        if (bind(server.socket_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            /* getting ip address  and port*/
            struct sockaddr_in my_addr;
            socklen_t len = sizeof(my_addr);
            getsockname(server.socket_fd, (struct sockaddr *) &my_addr, &len);
            inet_ntop(AF_INET, &my_addr.sin_addr, server.ip_addr, INET_ADDRSTRLEN);
            server.port_num = ntohs(my_addr.sin_port);
            freeaddrinfo(result);

            if ((listen(server.socket_fd, SOMAXCONN)) != 0) {
                perror("listen()");
                exit(EXIT_FAILURE);
            }

            printf("Listening on %s:%hu\n", server.ip_addr, server.port_num);

            return server;
        }

        close(server.socket_fd);
    }

    perror("bind()");
    exit(EXIT_FAILURE);
}

void server_disconnect_user(struct server *server, struct user_node *user_to_disconnect) {
    printf("%s (%s:%hu) on socket %d has disconnected from server\n", user_to_disconnect->nickname, user_to_disconnect->ip_addr, user_to_disconnect->port_num, user_to_disconnect->socket_fd);

    /* if user is in a chatroom
       we remove the user from the chatroom */
    if (user_to_disconnect->is_in_chatroom == 1) {
        /* getting the chatroom in which the user is */
        struct chatroom **chatroom = chatroom_get_by_user(server->list_chatrooms, user_to_disconnect);
        chatroom_remove_user(*chatroom, user_to_disconnect);

        /* informing all the users in the chatroom that a user has quit the chatroom */
        for (int l = 0; l < NUM_MAX_USERS; l++) {
            if ((*chatroom)->list_users[l] != NULL) {

                char payload[MSG_LEN];
                sprintf(payload, "%s has quit the channel", user_to_disconnect->nickname);

                struct packet res_packet = packet_init("SERVER", DEFAULT, "", payload, strlen(payload));
                packet_send(&res_packet, (*chatroom)->list_users[l]->socket_fd);
            }
        }

        /* deleting the chatroom if chatroom is now empty */
        if ((*chatroom)->num_users == 0) {
            chatroom_free(*chatroom);
            *chatroom = NULL;
        }
    }

    /* Close socket and set struct pollfd back to default */
    user_node_remove(&server->user_head, user_to_disconnect);
    server->num_users--;
}

/* getting user by its name */
struct user_node *server_get_user(struct server *server, char *nickname_user) {
    for (struct user_node *current = server->user_head; current != NULL; current = current->next) {
        if (strcmp(current->nickname, nickname_user) == 0) {
            return current;
        }
    }

    return NULL;
}

void server_handle_nickname_new_req(struct server *server, struct packet *req_packet) {

    for (struct user_node *user = server->user_head; user != NULL; user = user->next) {
        /* looking if nickname used by another user */
        if (strcmp(user->nickname, req_packet->header.infos) == 0) {
            char payload[] = "Nickname used by another user";
            struct packet res_packet = packet_init("SERVER", DEFAULT, req_packet->header.infos, payload, strlen(payload));
            packet_send(&res_packet, server->current_user->socket_fd);

            return;
        }
    }

    /* if nickname not used yet, adding user nickname */
    strcpy(server->current_user->nickname, req_packet->header.infos);

    if (!server->current_user->is_logged_in) {

        char payload[MSG_LEN];
        sprintf(payload, "Welcome on the chat %s, type a command or type /help if you need help", server->current_user->nickname);

        struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
        packet_send(&res_packet, server->current_user->socket_fd);

        server->current_user->is_logged_in = 1;
    } else {
        char payload[MSG_LEN];
        sprintf(payload, "You changed your nickname, your nickname is now %s", server->current_user->nickname);

        struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
        packet_send(&res_packet, server->current_user->socket_fd);
    }
}

void server_handle_nickname_list_req(struct server *server, struct packet *req_packet) {

    char payload[MSG_LEN];
    strcpy(payload, "Online users are:\n");

    for (struct user_node *user = server->user_head; user != NULL; user = user->next) {
        sprintf(payload + strlen(payload), "- %s\n", user->nickname);
    }

    payload[strlen(payload) - 1] = '\0';

    struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
    packet_send(&res_packet, server->current_user->socket_fd);
}

void server_handle_nickname_infos_req(struct server *server, struct packet *req_packet) {
    /* getting user we want information of */
    struct user_node *dest_user = server_get_user(server, req_packet->header.infos);

    /* if user does not exist */
    if (dest_user == NULL) {
        char payload[] = "Unknown user";
        struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
        packet_send(&res_packet, server->current_user->socket_fd);
    } else { /* if user exists */
        char payload[MSG_LEN];
        sprintf(payload, "%s is connected since %s with IP address %s and port number %hu", dest_user->nickname, dest_user->date, dest_user->ip_addr, dest_user->port_num);

        struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
        packet_send(&res_packet, server->current_user->socket_fd);
    }
}

void server_handle_broadcast_send_req(struct server *server, struct packet *req_packet) {

    for (struct user_node *dest_user = server->user_head; dest_user != NULL; dest_user = dest_user->next) {
        if (strcmp(dest_user->nickname, server->current_user->nickname) == 0) {
            struct packet res_packet = packet_init("me@all", req_packet->header.type, req_packet->header.infos, req_packet->payload, strlen(req_packet->payload));
            packet_send(&res_packet, server->current_user->socket_fd);
        } else {
            char from[2 * NICK_LEN];
            sprintf(from, "%s@all", server->current_user->nickname);

            struct packet res_packet = packet_init(from, req_packet->header.type, req_packet->header.infos, req_packet->payload, strlen(req_packet->payload));
            packet_send(&res_packet, dest_user->socket_fd);
        }
    }
}

void server_handle_unicast_send_req(struct server *server, struct packet *req_packet) {
    struct user_node *dest_user = server_get_user(server, req_packet->header.infos);

    /* if dest user exists */
    if (dest_user != NULL) {

        if (dest_user == server->current_user) {
            char payload[] = "You cannot send a message to yourself";
            struct packet res_packet = packet_init("SERVER", DEFAULT, req_packet->header.infos, payload, strlen(payload));
            packet_send(&res_packet, dest_user->socket_fd);
        } else {
            struct packet res_packet = packet_init(req_packet->header.from, req_packet->header.type, req_packet->header.infos, req_packet->payload, strlen(req_packet->payload));
            packet_send(&res_packet, dest_user->socket_fd);

            char from[2 * NICK_LEN];
            sprintf(from, "me@%s", dest_user->nickname);

            packet_set(&res_packet, from, req_packet->header.type, req_packet->header.infos, req_packet->payload, strlen(req_packet->payload));
            packet_send(&res_packet, server->current_user->socket_fd);
        }
    } else {
        /* if dest user does not exist */
        char payload[MSG_LEN];
        sprintf(payload, "User %s does not exist", req_packet->header.infos);

        struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
        packet_send(&res_packet, server->current_user->socket_fd);
    }
}

void server_handle_multicast_create_req(struct server *server, struct packet *req_packet) {
    /* we check if chatroom name is not used yet */
    if (server->num_chatrooms != 0) {
        for (int j = 0; j < NUM_MAX_CHATROOMS; j++) {
            if (server->list_chatrooms[j] != NULL) {
                /* if chatroom name exists yet */
                if (strcmp(server->list_chatrooms[j]->name, req_packet->header.infos) == 0) {
                    char payload[] = "Chat room name used by other users, please choose a new chatroom name";
                    struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                    packet_send(&res_packet, server->current_user->socket_fd);

                    return;
                }
            }
        }

        /* if chatrooom's name does not exist yet */
        /* we remove the user from all the chatrooms he is in before creating a new chatroom */
        struct chatroom **chatroom = chatroom_get_by_user(server->list_chatrooms, server->current_user);

        if (chatroom != NULL) {
            chatroom_remove_user(*chatroom, server->current_user);
            server->current_user->is_in_chatroom = 0;

            char payload[MSG_LEN];
            sprintf(payload, "You have quit the channel %s", (*chatroom)->name);

            struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
            packet_send(&res_packet, server->current_user->socket_fd);

            for (int l = 0; l < NUM_MAX_USERS; l++) {
                if ((*chatroom)->list_users[l] != NULL) {
                    sprintf(payload, "%s has quit the channel", server->current_user->nickname);

                    char from[2 * NICK_LEN];
                    sprintf(from, "SERVER@%s", (*chatroom)->name);

                    packet_set(&res_packet, from, req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                    packet_send(&res_packet, (*chatroom)->list_users[l]->socket_fd);
                }
            }

            /* if chatroom is empty, destroying it */
            if ((*chatroom)->num_users == 0) {
                sprintf(res_packet.payload, "You were the last user in %s, this channel has been destroyed", (*chatroom)->name);

                packet_set(&res_packet, "SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                packet_send(&res_packet, server->current_user->socket_fd);

                chatroom_free(*chatroom);
                *chatroom = NULL;
                server->num_chatrooms--;
            }
        }
    }

    /* creating a new chatroom */
    for (int j = 0; j < NUM_MAX_CHATROOMS; j++) {
        if (server->list_chatrooms[j] == NULL) {
            struct chatroom *new_chatroom = chatroom_init(req_packet->header.infos, server->current_user);
            server->list_chatrooms[j] = new_chatroom;
            server->num_chatrooms++;

            char payload[MSG_LEN];
            sprintf(payload, "You have created channel %s", new_chatroom->name);

            struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
            packet_send(&res_packet, server->current_user->socket_fd);

            return;
        }
    }
}

void server_handle_multicast_list_req(struct server *server, struct packet *req_packet) {
    if (server->num_chatrooms != 0) {

        char payload[MSG_LEN];
        strcpy(payload, "Online chatrooms are:\n");

        for (int j = 0; j < NUM_MAX_CHATROOMS; j++) {
            if (server->list_chatrooms[j] != NULL) {
                sprintf(payload + strlen(payload), "- %s\n", server->list_chatrooms[j]->name);
            }
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
        /* getting chatroom user wants to join */
        struct chatroom **chatroom_to_join = chatroom_get_by_name(server->list_chatrooms, req_packet->header.infos);

        /* if chatroom exists */
        if (chatroom_to_join != NULL) {
            struct chatroom **current_chatroom = chatroom_get_by_user(server->list_chatrooms, server->current_user);

            /* if user is in a chatroom*/
            if (current_chatroom != NULL) {
                /* if user is yet in the chatroom */
                if (current_chatroom == chatroom_to_join) {

                    for (int j = 0; j < NUM_MAX_USERS; j++) {
                        if ((*chatroom_to_join)->list_users[j] == server->current_user) {

                            char payload[MSG_LEN];
                            strcpy(payload, "You were yet in this chatroom");

                            struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                            packet_send(&res_packet, server->current_user->socket_fd);

                            return;
                        }
                    }
                } else {

                    for (int l = 0; l < NUM_MAX_USERS; l++) {
                        if ((*current_chatroom)->list_users[l] != NULL) {

                            char payload[MSG_LEN];
                            sprintf(payload, "%s has quit the channel", server->current_user->nickname);

                            char from[2 * NICK_LEN];
                            sprintf(from, "SERVER@%s", (*current_chatroom)->name);

                            struct packet res_packet = packet_init(from, req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                            packet_send(&res_packet, (*current_chatroom)->list_users[l]->socket_fd);
                        }
                    }

                    chatroom_remove_user(*current_chatroom, server->current_user);
                    server->current_user->is_in_chatroom = 0;

                    char payload[MSG_LEN];
                    sprintf(payload, "You have quit the channel %s", (*current_chatroom)->name);

                    struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                    packet_send(&res_packet, server->current_user->socket_fd);

                    /* if chatroom is empty, destroying it */
                    if ((*current_chatroom)->num_users == 0) {
                        sprintf(payload, "You were the last user in %s, this channel has been destroyed", (*current_chatroom)->name);

                        packet_set(&res_packet, "SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                        packet_send(&res_packet, server->current_user->socket_fd);

                        chatroom_free(*current_chatroom);
                        *current_chatroom = NULL;
                        server->num_chatrooms--;
                    }
                }
            }

            /* joining chatroom */
            int is_joined = 0;

            for (int k = 0; k < NUM_MAX_USERS; k++) {
                if ((*chatroom_to_join)->list_users[k] == NULL && is_joined == 0) {

                    (*chatroom_to_join)->list_users[k] = server->current_user;
                    server->current_user->is_in_chatroom = 1;
                    (*chatroom_to_join)->num_users++;

                    char payload[MSG_LEN];
                    sprintf(payload, "You have joined %s", req_packet->header.infos);

                    struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                    packet_send(&res_packet, server->current_user->socket_fd);

                    is_joined = 1;
                } else if ((*chatroom_to_join)->list_users[k] != NULL) {
                    /* informing the other users in the chatroom that a new user joined*/
                    char payload[MSG_LEN];
                    sprintf(payload, "%s has joined the channel", server->current_user->nickname);

                    char from[2 * NICK_LEN];
                    sprintf(from, "SERVER@%s", (*chatroom_to_join)->name);

                    struct packet res_packet = packet_init(from, req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                    packet_send(&res_packet, (*chatroom_to_join)->list_users[k]->socket_fd);
                }
            }

            return;
        }
    }

    /* if chatroom does not exist */
    char payload[MSG_LEN];
    sprintf(payload, "Channel %s does not exist", req_packet->header.infos);

    struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
    packet_send(&res_packet, server->current_user->socket_fd);
}

void server_handle_multicast_quit_req(struct server *server, struct packet *req_packet) {
    /* getting chatroom user wants to quit */
    if (server->num_chatrooms != 0) {
        struct chatroom **chatroom = chatroom_get_by_name(server->list_chatrooms, req_packet->header.infos);

        if (chatroom != NULL) {
            struct user_node **user = chatroom_remove_user(*chatroom, server->current_user);

            /* if chatroom exists */
            if (user != NULL) {

                for (int l = 0; l < NUM_MAX_USERS; l++) {
                    if ((*chatroom)->list_users[l] != NULL) {

                        char payload[MSG_LEN];
                        sprintf(payload, "%s has quit the channel", server->current_user->nickname);

                        char from[2 * NICK_LEN];
                        sprintf(from, "SERVER@%s", (*chatroom)->name);

                        struct packet res_packet = packet_init(from, req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                        packet_send(&res_packet, (*chatroom)->list_users[l]->socket_fd);
                    }
                }

                server->current_user->is_in_chatroom = 0;

                char payload[MSG_LEN];
                sprintf(payload, "You have quit the channel %s", (*chatroom)->name);

                struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                packet_send(&res_packet, server->current_user->socket_fd);

                /* if chatroom is now empty */
                if ((*chatroom)->num_users == 0) {
                    sprintf(payload, "You were the last user in this channel, %s has been destroyed", (*chatroom)->name);

                    packet_set(&res_packet, "SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                    packet_send(&res_packet, server->current_user->socket_fd);

                    chatroom_free(*chatroom);
                    *chatroom = NULL;
                    server->num_chatrooms--;
                }
            } else {
                /* if user was not in the chatroom */
                char payload[MSG_LEN];
                sprintf(payload, "You were not in chatroom %s", (*chatroom)->name);

                struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
                packet_send(&res_packet, server->current_user->socket_fd);
            }

            return;
        }
    }

    /* if chatroom does not exist */
    char payload[MSG_LEN];
    sprintf(payload, "Channel %s does not exist", req_packet->header.infos);

    struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
    packet_send(&res_packet, server->current_user->socket_fd);
}

void server_handle_multicast_send_req(struct server *server, struct packet *req_packet) {
    /* if user is not in a chatroom*/
    if (server->current_user->is_in_chatroom == 0) {

        struct packet res_packet = packet_init("SERVER", ECHO_SEND, req_packet->header.infos, req_packet->payload, strlen(req_packet->payload));
        packet_send(&res_packet, server->current_user->socket_fd);

    } else {
        /* getting the chatroom the user wants to send the message in */
        struct chatroom **chatroom = chatroom_get_by_user(server->list_chatrooms, server->current_user);

        /* sending message to all the users in the chatroom */
        for (int l = 0; l < NUM_MAX_USERS; l++) {
            if ((*chatroom)->list_users[l] != NULL) {
                if (strcmp((*chatroom)->list_users[l]->nickname, server->current_user->nickname) == 0) {

                    char from[2 * NICK_LEN];
                    sprintf(from, "me@%s", (*chatroom)->name);

                    struct packet res_packet = packet_init(from, req_packet->header.type, req_packet->header.infos, req_packet->payload, strlen(req_packet->payload));
                    packet_send(&res_packet, (*chatroom)->list_users[l]->socket_fd);
                } else {

                    char from[2 * NICK_LEN];
                    sprintf(from, "%s@%s", server->current_user->nickname, (*chatroom)->name);

                    struct packet res_packet = packet_init(from, req_packet->header.type, req_packet->header.infos, req_packet->payload, strlen(req_packet->payload));
                    packet_send(&res_packet, (*chatroom)->list_users[l]->socket_fd);
                }
            }
        }
    }
}

void server_handle_file_req(struct server *server, struct packet *req_packet) {
    /* getting the user he wants to send file to */
    struct user_node *dest_user = server_get_user(server, req_packet->header.infos);

    /* if user does not exist */
    if (dest_user == NULL) {

        char payload[MSG_LEN];
        sprintf(payload, "%s does not exist", req_packet->header.infos);

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
    struct user_node *dest_user = server_get_user(server, req_packet->header.infos);

    struct packet res_packet = packet_init(req_packet->header.from, req_packet->header.type, req_packet->header.infos, req_packet->payload, strlen(req_packet->payload));
    packet_send(&res_packet, dest_user->socket_fd);
}

void server_handle_file_reject_req(struct server *server, struct packet *req_packet) {
    struct user_node *dest_user = server_get_user(server, req_packet->header.infos);

    char payload[MSG_LEN];
    sprintf(payload, "%s cancelled file transfer", server->current_user->nickname);

    struct packet res_packet = packet_init("SERVER", req_packet->header.type, req_packet->header.infos, payload, strlen(payload));
    packet_send(&res_packet, dest_user->socket_fd);
}