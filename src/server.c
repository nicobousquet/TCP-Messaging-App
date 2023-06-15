#include "../include/server.h"
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

struct server *server_init(char *port) {
    struct server *server = malloc(sizeof(struct server));
    memset(server, 0, sizeof(struct server));
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, port, &hints, &result) != 0) {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        server->socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (server->socket_fd == -1) {
            continue;
        }
        if (bind(server->socket_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            /* getting ip address  and port*/
            struct sockaddr_in my_addr;
            socklen_t len = sizeof(my_addr);
            getsockname(server->socket_fd, (struct sockaddr *) &my_addr, &len);
            inet_ntop(AF_INET, &my_addr.sin_addr, server->ip_addr, 16);
            server->port_num = ntohs(my_addr.sin_port);
            freeaddrinfo(result);
            if ((listen(server->socket_fd, SOMAXCONN)) != 0) {
                free(server);
                perror("listen()");
                exit(EXIT_FAILURE);
            }
            printf("Listening on %s:%hu\n", server->ip_addr, server->port_num);
            return server;
        }
        close(server->socket_fd);
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
        server->num_users--;
        /* informing all the users in the chatroom that a user has quit the chatroom */
        for (int l = 0; l < NUM_MAX_USERS; l++) {
            if ((*chatroom)->list_users[l] != NULL) {
                sprintf(server->packet.payload, "%s has quit the channel", user_to_disconnect->nickname);
                server->packet.header.len_payload = strlen(server->packet.payload);
                strcpy(server->packet.header.from, "SERVER");
                packet_send(&server->packet, (*chatroom)->list_users[l]->socket_fd);
            }
        }
        /* deleting the chatroom if chatroom is now empty */
        if ((*chatroom)->num_users == 0) {
            free(*chatroom);
            *chatroom = NULL;
        }
    }
    /* Close socket and set struct pollfd back to default */
    user_node_remove(&server->linked_list_users, user_to_disconnect);
}

/* getting user by its name */
struct user_node *server_get_user(struct server *server, char *nickname_user) {
    for (struct user_node *current = server->linked_list_users; current != NULL; current = current->next) {
        if (strcmp(current->nickname, nickname_user) == 0) {
            return current;
        }
    }
    return NULL;
}

void server_handle_nickname_new_req(struct server *server) {
    strcpy(server->packet.header.from, "SERVER");
    for (struct user_node *user = server->linked_list_users; user != NULL; user = user->next) {
        /* looking if nickname used by another user */
        if (strcmp(user->nickname, server->packet.header.infos) == 0) {
            strcpy(server->packet.payload, "Nickname used by another user");
            server->packet.header.len_payload = strlen(server->packet.payload);
            server->packet.header.type = DEFAULT;
            packet_send(&server->packet, server->current_user->socket_fd);
            return;
        }
    }
    /* if nickname not used yet, adding user nickname */
    strcpy(server->current_user->nickname, server->packet.header.infos);
    if (!server->current_user->is_logged_in) {
        sprintf(server->packet.payload, "Welcome on the chat %s, type a command or type /help if you need help", server->current_user->nickname);
        server->current_user->is_logged_in = 1;
    } else {
        sprintf(server->packet.payload, "You changed your nickname, your nickname is now %s", server->current_user->nickname);
    }

    server->packet.header.len_payload = strlen(server->packet.payload);
    packet_send(&server->packet, server->current_user->socket_fd);
}

void server_handle_nickname_list_req(struct server *server) {
    strcpy(server->packet.header.from, "SERVER");
    strcpy(server->packet.payload, "Online users are:\n");
    for (struct user_node *user = server->linked_list_users; user != NULL; user = user->next) {
        sprintf(server->packet.payload + strlen(server->packet.payload), "- %s\n", user->nickname);
    }
    server->packet.payload[strlen(server->packet.payload) - 1] = '\0';
    server->packet.header.len_payload = strlen(server->packet.payload);
    packet_send(&server->packet, server->current_user->socket_fd);
}

void server_handle_nickname_infos_req(struct server *server) {
    strcpy(server->packet.header.from, "SERVER");
    /* getting user we want information of */
    struct user_node *nickname_dest = server_get_user(server, server->packet.header.infos);
    /* if user does not exist */
    if (nickname_dest == NULL) {
        strcpy(server->packet.payload, "Unknown user");
        server->packet.header.len_payload = strlen(server->packet.payload);
    } else { /* if user exists */
        sprintf(server->packet.payload, "%s is connected since %s with IP address %s and port number %hu", nickname_dest->nickname, nickname_dest->date, nickname_dest->ip_addr, nickname_dest->port_num);
        server->packet.header.len_payload = strlen(server->packet.payload);
    }
    packet_send(&server->packet, server->current_user->socket_fd);
}

void server_handle_broadcast_send_req(struct server *server) {
    for (struct user_node *nickname_dest = server->linked_list_users; nickname_dest != NULL; nickname_dest = nickname_dest->next) {
        if (strcmp(nickname_dest->nickname, server->current_user->nickname) == 0) {
            strcpy(server->packet.header.from, "me@all");
        } else {
            sprintf(server->packet.header.from, "%s@all", server->current_user->nickname);
        }
        packet_send(&server->packet, nickname_dest->socket_fd);
    }
}

void server_handle_unicast_send_req(struct server *server) {
    struct user_node *nickname_dest = server_get_user(server, server->packet.header.infos);
    /* if dest user exists */
    if (nickname_dest != NULL) {
        packet_send(&server->packet, nickname_dest->socket_fd);
        sprintf(server->packet.header.from, "me@%s", nickname_dest->nickname);
        packet_send(&server->packet, server->current_user->socket_fd);
    } else {
        /* if dest user does not exist */
        sprintf(server->packet.payload, "User %s does not exist", server->packet.header.infos);
        strcpy(server->packet.header.from, "SERVER");
        server->packet.header.len_payload = strlen(server->packet.payload);
        packet_send(&server->packet, server->current_user->socket_fd);
    }
}

void server_handle_multicast_create_req(struct server *server) {
    /* we check if chatroom name is not used yet */
    if (server->num_chatrooms != 0) {
        for (int j = 0; j < NUM_MAX_CHATROOMS; j++) {
            if (server->list_chatrooms[j] != NULL) {
                /* if chatroom name exists yet */
                if (strcmp(server->list_chatrooms[j]->name, server->packet.header.infos) == 0) {
                    strcpy(server->packet.payload, "Chat room name used by other users, please choose a new chatroom name");
                    server->packet.header.len_payload = strlen(server->packet.payload);
                    strcpy(server->packet.header.from, "SERVER");
                    packet_send(&server->packet, server->current_user->socket_fd);
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
            sprintf(server->packet.payload, "You have quit the channel %s", (*chatroom)->name);
            server->packet.header.len_payload = strlen(server->packet.payload);
            strcpy(server->packet.header.from, "SERVER");
            packet_send(&server->packet, server->current_user->socket_fd);
            for (int l = 0; l < NUM_MAX_USERS; l++) {
                if ((*chatroom)->list_users[l] != NULL) {
                    sprintf(server->packet.payload, "%s has quit the channel", server->current_user->nickname);
                    server->packet.header.len_payload = strlen(server->packet.payload);
                    sprintf(server->packet.header.from, "SERVER@%s", (*chatroom)->name);
                    packet_send(&server->packet, (*chatroom)->list_users[l]->socket_fd);
                }
            }
            /* if chatroom is empty, destroying it */
            if ((*chatroom)->num_users == 0) {
                sprintf(server->packet.payload, "You were the last user in %s, this channel has been destroyed", (*chatroom)->name);
                server->packet.header.len_payload = strlen(server->packet.payload);
                strcpy(server->packet.header.from, "SERVER");
                packet_send(&server->packet, server->current_user->socket_fd);
                free(*chatroom);
                *chatroom = NULL;
                server->num_chatrooms--;
            }
        }
    }

    /* creating a new chatroom */
    for (int j = 0; j < NUM_MAX_CHATROOMS; j++) {
        if (server->list_chatrooms[j] == NULL) {
            struct chatroom *new_chatroom = chatroom_init(server->packet.header.infos, server->current_user);
            server->list_chatrooms[j] = new_chatroom;
            server->num_chatrooms++;
            sprintf(server->packet.payload, "You have created channel %s", new_chatroom->name);
            server->packet.header.len_payload = strlen(server->packet.payload);
            strcpy(server->packet.header.from, "SERVER");
            packet_send(&server->packet, server->current_user->socket_fd);
            return;
        }
    }
}

void server_handle_multicast_list_req(struct server *server) {
    if (server->num_chatrooms != 0) {
        strcpy(server->packet.payload, "Online chatrooms are:\n");
        for (int j = 0; j < NUM_MAX_CHATROOMS; j++) {
            if (server->list_chatrooms[j] != NULL) {
                sprintf(server->packet.payload + strlen(server->packet.payload), "- %s\n", server->list_chatrooms[j]->name);
            }
        }
        server->packet.payload[strlen(server->packet.payload) - 1] = '\0';
        server->packet.header.len_payload = strlen(server->packet.payload);
        strcpy(server->packet.header.from, "SERVER");
        packet_send(&server->packet, server->current_user->socket_fd);
        return;
    }

    strcpy(server->packet.payload, "There is no chatrooms created\n");
    server->packet.payload[strlen(server->packet.payload) - 1] = '\0';
    server->packet.header.len_payload = strlen(server->packet.payload);
    strcpy(server->packet.header.from, "SERVER");
    packet_send(&server->packet, server->current_user->socket_fd);
}

void server_handle_multicast_join_req(struct server *server) {
    if (server->num_chatrooms != 0) {
        /* getting chatroom user wants to join */
        struct chatroom **chatroom_to_join = chatroom_get_by_name(server->list_chatrooms, server->packet.header.infos);
        /* if chatroom exists */
        if (chatroom_to_join != NULL) {
            struct chatroom **current_chatroom = chatroom_get_by_user(server->list_chatrooms, server->current_user);
            /* if user is in a chatroom*/
            if (current_chatroom != NULL) {
                /* if user is yet in the chatroom */
                if (current_chatroom == chatroom_to_join) {
                    for (int j = 0; j < NUM_MAX_USERS; j++) {
                        if ((*chatroom_to_join)->list_users[j] == server->current_user) {
                            strcpy(server->packet.payload, "You were yet in this chatroom");
                            server->packet.header.len_payload = strlen(server->packet.payload);
                            strcpy(server->packet.header.from, "SERVER");
                            packet_send(&server->packet, server->current_user->socket_fd);
                            return;
                        }
                    }
                } else {
                    chatroom_remove_user(*current_chatroom, server->current_user);
                    server->current_user->is_in_chatroom = 0;
                    sprintf(server->packet.payload, "You have quit the channel %s", (*current_chatroom)->name);
                    server->packet.header.len_payload = strlen(server->packet.payload);
                    strcpy(server->packet.header.from, "SERVER");
                    packet_send(&server->packet, server->current_user->socket_fd);
                    for (int l = 0; l < NUM_MAX_USERS; l++) {
                        if ((*current_chatroom)->list_users[l] != NULL) {
                            sprintf(server->packet.payload, "%s has quit the channel", server->current_user->nickname);
                            server->packet.header.len_payload = strlen(server->packet.payload);
                            sprintf(server->packet.header.from, "SERVER@%s", (*current_chatroom)->name);
                            packet_send(&server->packet, (*current_chatroom)->list_users[l]->socket_fd);
                        }
                    }
                    /* if chatroom is empty, destroying it */
                    if ((*current_chatroom)->num_users == 0) {
                        sprintf(server->packet.payload, "You were the last user in %s, this channel has been destroyed", (*current_chatroom)->name);
                        server->packet.header.len_payload = strlen(server->packet.payload);
                        strcpy(server->packet.header.from, "SERVER");
                        packet_send(&server->packet, server->current_user->socket_fd);
                        free(*current_chatroom);
                        *current_chatroom = NULL;
                        server->num_chatrooms--;
                    }
                }
            }
            /* joining chatroom */
            int joined = 0;
            for (int k = 0; k < NUM_MAX_USERS; k++) {
                if ((*chatroom_to_join)->list_users[k] == NULL && joined == 0) {
                    (*chatroom_to_join)->list_users[k] = server->current_user;
                    server->current_user->is_in_chatroom = 1;
                    (*chatroom_to_join)->num_users++;
                    sprintf(server->packet.payload, "You have joined %s", server->packet.header.infos);
                    server->packet.header.len_payload = strlen(server->packet.payload);
                    strcpy(server->packet.header.from, "SERVER");
                    packet_send(&server->packet, server->current_user->socket_fd);
                    joined = 1;
                } else if ((*chatroom_to_join)->list_users[k] != NULL) {
                    /* informing the other users in the chatroom that a new user joined*/
                    sprintf(server->packet.payload, "%s has joined the channel", server->current_user->nickname);
                    server->packet.header.len_payload = strlen(server->packet.payload);
                    sprintf(server->packet.header.from, "SERVER@%s", (*chatroom_to_join)->name);
                    packet_send(&server->packet, (*chatroom_to_join)->list_users[k]->socket_fd);
                }
            }
            return;
        }
    }
    /* if chatroom does not exist */
    sprintf(server->packet.payload, "Channel %s does not exist", server->packet.header.infos);
    server->packet.header.len_payload = strlen(server->packet.payload);
    strcpy(server->packet.header.from, "SERVER");
    packet_send(&server->packet, server->current_user->socket_fd);
    return;
}

void server_handle_multicast_quit_req(struct server *server) {
    /* getting chatroom user wants to quit */
    if (server->num_chatrooms != 0) {
        struct chatroom **chatroom = chatroom_get_by_name(server->list_chatrooms, server->packet.header.infos);
        if (chatroom != NULL) {
            struct user_node **user = chatroom_remove_user(*chatroom, server->current_user);
            /* if chatroom exists */
            if (user != NULL) {
                server->current_user->is_in_chatroom = 0;
                sprintf(server->packet.payload, "You have quit the channel %s", (*chatroom)->name);
                server->packet.header.len_payload = strlen(server->packet.payload);
                strcpy(server->packet.header.from, "SERVER");
                packet_send(&server->packet, server->current_user->socket_fd);

                for (int l = 0; l < NUM_MAX_USERS; l++) {
                    if ((*chatroom)->list_users[l] != NULL) {
                        sprintf(server->packet.payload, "%s has quit the channel", server->current_user->nickname);
                        server->packet.header.len_payload = strlen(server->packet.payload);
                        sprintf(server->packet.header.from, "SERVER@%s", (*chatroom)->name);
                        packet_send(&server->packet, (*chatroom)->list_users[l]->socket_fd);
                    }
                }
                /* if chatroom is now empty */
                if ((*chatroom)->num_users == 0) {
                    sprintf(server->packet.payload, "You were the last user in this channel, %s has been destroyed", (*chatroom)->name);
                    server->packet.header.len_payload = strlen(server->packet.payload);
                    strcpy(server->packet.header.from, "SERVER");
                    packet_send(&server->packet, server->current_user->socket_fd);
                    free(*chatroom);
                    *chatroom = NULL;
                    server->num_chatrooms--;
                }
            } else {
                /* if user was not in the chatroom */
                sprintf(server->packet.payload, "You were not in chatroom %s", (*chatroom)->name);
                server->packet.header.len_payload = strlen(server->packet.payload);
                strcpy(server->packet.header.from, "SERVER");
                packet_send(&server->packet, server->current_user->socket_fd);
            }
            return;
        }
    }
    /* if chatroom does not exist */
    sprintf(server->packet.payload, "Channel %s does not exist", server->packet.header.infos);
    server->packet.header.len_payload = strlen(server->packet.payload);
    strcpy(server->packet.header.from, "SERVER");
    packet_send(&server->packet, server->current_user->socket_fd);
}

void server_handle_multicast_send_req(struct server *server) {
    /* if user is not in a chatroom*/
    if (server->current_user->is_in_chatroom == 0) {
        /* message type is ECHO_SEND*/
        server->packet.header.type = ECHO_SEND;
        strcpy(server->packet.header.from, "SERVER");
        packet_send(&server->packet, server->current_user->socket_fd);
    } else {
        /* getting the chatroom the user wants to send the message in */
        struct chatroom **chatroom = chatroom_get_by_user(server->list_chatrooms, server->current_user);
        /* sending message to all the users in the chatroom */
        for (int l = 0; l < NUM_MAX_USERS; l++) {
            if ((*chatroom)->list_users[l] != NULL) {
                if (strcmp((*chatroom)->list_users[l]->nickname, server->current_user->nickname) == 0) {
                    sprintf(server->packet.header.from, "me@%s", (*chatroom)->name);
                } else {
                    sprintf(server->packet.header.from, "%s@%s", server->current_user->nickname, (*chatroom)->name);
                }
                packet_send(&server->packet, (*chatroom)->list_users[l]->socket_fd);
            }
        }
    }
}

void server_handle_file_req(struct server *server) {
    /* getting the user he wants to send file to */
    struct user_node *nickname_dest = server_get_user(server, server->packet.header.infos);
    /* if user does not exist */
    if (nickname_dest == NULL) {
        server->packet.header.type = DEFAULT;
        sprintf(server->packet.payload, "%s does not exist", server->packet.header.infos);
        server->packet.header.len_payload = strlen(server->packet.payload);
        strcpy(server->packet.header.from, "SERVER");
        packet_send(&server->packet, server->current_user->socket_fd);
    } else if (nickname_dest == server->current_user) {
        server->packet.header.type = DEFAULT;
        strcpy(server->packet.payload, "You cannot send a file to yourself");
        server->packet.header.len_payload = strlen(server->packet.payload);
        strcpy(server->packet.header.from, "SERVER");
        packet_send(&server->packet, server->current_user->socket_fd);
    } else {
        strcpy(server->packet.header.infos, server->packet.header.from);
        server->packet.header.len_payload = strlen(server->packet.payload);
        strcpy(server->packet.header.from, "SERVER");
        packet_send(&server->packet, nickname_dest->socket_fd);
    }
}

void server_handle_file_accept_req(struct server *server) {
    struct user_node *nickname_dest = server_get_user(server, server->packet.header.infos);
    packet_send(&server->packet, nickname_dest->socket_fd);
}

void server_handle_file_reject_req(struct server *server) {
    struct user_node *nickname_dest = server_get_user(server, server->packet.header.infos);
    sprintf(server->packet.payload, "%s cancelled file transfer", server->current_user->nickname);
    server->packet.header.len_payload = strlen(server->packet.payload);
    strcpy(server->packet.header.from, "SERVER");
    packet_send(&server->packet, nickname_dest->socket_fd);
}