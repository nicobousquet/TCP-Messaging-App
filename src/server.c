#include "../include/server.h"
#include <poll.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>

/* adding user in the list */
void pushUser(struct server *server, struct socket *socket, char *pseudo, char *date, int inChatroom) {
    user_t *newNode = malloc(sizeof(user_t));
    memcpy(&newNode->socket, socket, sizeof(struct socket));
    strcpy(newNode->pseudo, pseudo);
    strcpy(newNode->date, date);
    newNode->inChatroom = inChatroom;
    newNode->next = server->users;
    server->users = newNode;
}

/* freeing user node in the list */
void freeUser(struct server *server, int fd) {
    user_t *newNode = malloc(sizeof(user_t));
    newNode->next = server->users;
    server->users = newNode;
    user_t *current;
    /* looking for the user we want to free */
    for (current = server->users; current->next != NULL; current = current->next) {
        if ((current->next)->socket.fd == fd) {
            break;
        }
    }
    /* freeing user */
    user_t *tmp = current->next;
    current->next = (current->next)->next;
    free(tmp);
    /* freeing fake first node of the list */
    tmp = server->users;
    server->users = (server->users)->next;
    free(tmp);
}

typedef struct struct1 {
    struct server *server;
    user_t *user;
} struct1_t;

typedef struct struct2 {
    struct server *server;
    char *name;
} struct2_t;

/* getting chatroom */
chatroom_t **getChatroomInServer(void *arg1, int arg2) {
    /* getting chatroom by user */
    if (arg2 == 0) {
        struct1_t *ptr = (struct1_t *) arg1;
        struct server *server = ptr->server;
        user_t *user = ptr->user;
        for (int j = 0; j < NB_CHATROOMS; j++) {
            if (server->chatrooms[j] != NULL) {
                for (int k = 0; k < NB_USERS; k++) {
                    if (server->chatrooms[j]->users[k] != NULL) {
                        if (server->chatrooms[j]->users[k] == user) {
                            return &(server->chatrooms[j]);
                        }
                    }
                }
            }
        }
    } else if (arg2 == 1) {
        /* getting chatroom by name */
        struct2_t *ptr = (struct2_t *) arg1;
        struct server *server = ptr->server;
        char *name = ptr->name;
        for (int j = 0; j < NB_CHATROOMS; j++) {
            if (server->chatrooms[j] != NULL) {
                if (strcmp(server->chatrooms[j]->name, name) == 0) {
                    return &(server->chatrooms[j]);
                }
            }
        }
    }
    return NULL;
}

/* getting user in chatroom */
user_t **getUserInChatroom(chatroom_t **chatroom, user_t *user) {
    for (int k = 0; k < NB_USERS; k++) {
        if ((*chatroom)->users[k] != NULL && (*chatroom)->users[k] == user) {
            return &(*chatroom)->users[k];
        }
    }
    return NULL;
}

/* getting user by its name */
user_t *getUser(struct server *server, char *name) {
    for (user_t *current = server->users; current != NULL; current = current->next) {
        if (strcmp(name, current->pseudo) == 0) {
            return current;
        }
    }
    return NULL;
}

int processRequest(int clientFd, struct server *server) {
    user_t *currentUser = server->users;
    /* getting the user which is doing the request */
    while (currentUser->socket.fd != clientFd) {
        currentUser = currentUser->next;
    }
    /* Cleaning memory */
    memset(&server->msgStruct, 0, sizeof(struct message));
    memset(server->buffer, 0, MSG_LEN);
    /* Receiving structure */
    /* if client disconnected */
    if ((recv(clientFd, &server->msgStruct, sizeof(struct message), MSG_WAITALL)) <= 0) {
        printf("client on socket %d has disconnected from server\n", clientFd);
        /* if user is in a chatroom
         * we remove the user from the chatroom
         */
        if (currentUser->inChatroom == 1) {
            struct1_t arg1 = {server, currentUser};
            /* getting the chatroom in which the user is */
            chatroom_t **chatroom = getChatroomInServer((void *) &arg1, 0);
            /* getting the user in the chatroom */
            user_t **user = getUserInChatroom(chatroom, currentUser);
            /* removing the user in the chatroom */
            *user = NULL;
            (*chatroom)->nbOfUsers--;
            /* informing all the users in the chatroom that a user has quit the chatroom */
            for (int l = 0; l < NB_USERS; l++) {
                if ((*chatroom)->users[l] != NULL) {
                    sprintf(server->payload, "%s has quit the channel", currentUser->pseudo);
                    server->msgStruct.payloadLen = strlen(server->payload);
                    strcpy(server->msgStruct.nickSender, "SERVER");
                    sendMsg((*chatroom)->users[l]->socket.fd, &server->msgStruct, server->payload);
                }
            }
            /* deleting the chatroom if chatroom is now empty */
            if ((*chatroom)->nbOfUsers == 0) {
                free(*chatroom);
                *chatroom = NULL;
            }
        }
        /* Close socket and set struct pollfd back to default */
        freeUser(server, clientFd);
        return 0;
    }
    /* Receiving message */
    memset(server->buffer, 0, MSG_LEN);
    if (server->msgStruct.payloadLen != 0 && recv(clientFd, server->buffer, server->msgStruct.payloadLen, MSG_WAITALL) <= 0) {
        perror("recv");
        return 1;
    }
    printf("payloadLen: %ld / nickSender: %s / type: %s / infos: %s\n", server->msgStruct.payloadLen, server->msgStruct.nickSender, msgTypeStr[server->msgStruct.type], server->msgStruct.infos);
    printf("payload: %s\n", server->buffer);

    switch (server->msgStruct.type) {
        /* if user wants to change/create pseudo */
        case NICKNAME_NEW: {
            strcpy(server->msgStruct.nickSender, "SERVER");
            for (user_t *user = server->users; user != NULL; user = user->next) {
                /* looking if pseudo used by another user */
                if (strcmp(user->pseudo, server->msgStruct.infos) == 0 && user->socket.fd != clientFd) {
                    strcpy(server->payload, "Pseudo used by another user");
                    server->msgStruct.payloadLen = strlen(server->payload);
                    server->msgStruct.type = DEFAULT;
                    sendMsg(clientFd, &server->msgStruct, server->payload);
                    return 1;
                }
            }
            /* if pseudo not used yet, adding user pseudo */
            strcpy(currentUser->pseudo, server->msgStruct.infos);
            sprintf(server->payload, "Welcome on the chat %s, type a command or type /help if you need help", currentUser->pseudo);
            server->msgStruct.payloadLen = strlen(server->payload);
            sendMsg(clientFd, &server->msgStruct, server->payload);
            return 1;
        }
            /* if user wants to see the list of other connected users */
        case NICKNAME_LIST: {
            strcpy(server->msgStruct.nickSender, "SERVER");
            strcpy(server->payload, "Online users are:\n");
            for (user_t *user = server->users; user != NULL; user = user->next) {
                sprintf(server->payload + strlen(server->payload), "- %s\n", user->pseudo);
            }
            server->payload[strlen(server->payload) - 1] = '\0';
            server->msgStruct.payloadLen = strlen(server->payload);
            sendMsg(clientFd, &server->msgStruct, server->payload);
            return 1;
        }
            /* if user wants to know ip address, connection port number and connection date of another user */
        case NICKNAME_INFOS: {
            strcpy(server->msgStruct.nickSender, "SERVER");
            /* getting user we want information of */
            user_t *dstUser = getUser(server, server->msgStruct.infos);
            /* if user does not exist */
            if (dstUser == NULL) {
                strcpy(server->payload, "Unknown user");
                server->msgStruct.payloadLen = strlen(server->payload);
            } else { /* if user exists */
                sprintf(server->payload, "%s is connected since %s with IP address %s and port number %hu", dstUser->pseudo, dstUser->date, dstUser->socket.ipAddr, dstUser->socket.port);
                server->msgStruct.payloadLen = strlen(server->payload);
            }
            sendMsg(clientFd, &server->msgStruct, server->payload);
            return 1;
        }
            /* if user wants to send a message to all the users */
        case BROADCAST_SEND: {
            strcpy(server->payload, server->buffer);
            for (user_t *dstUser = server->users; dstUser != NULL; dstUser = dstUser->next) {
                if (strcmp(dstUser->pseudo, currentUser->pseudo) == 0) {
                    strcpy(server->msgStruct.nickSender, "me@all");
                } else {
                    sprintf(server->msgStruct.nickSender, "%s@all", currentUser->pseudo);
                }
                sendMsg(dstUser->socket.fd, &server->msgStruct, server->payload);
            }
            return 1;
        }
            /* if user wants to send a message to another specific user */
        case UNICAST_SEND: {
            strcpy(server->payload, server->buffer);
            user_t *dstUser = getUser(server, server->msgStruct.infos);
            /* if dest user exists */
            if (dstUser != NULL) {
                sendMsg(dstUser->socket.fd, &server->msgStruct, server->payload);
                sprintf(server->msgStruct.nickSender, "me@%s", dstUser->pseudo);
                sendMsg(currentUser->socket.fd, &server->msgStruct, server->payload);
            } else {
                /* if dest user does not exist */
                sprintf(server->payload, "User %s does not exist", server->msgStruct.infos);
                strcpy(server->msgStruct.nickSender, "SERVER");
                server->msgStruct.payloadLen = strlen(server->payload);
                sendMsg(clientFd, &server->msgStruct, server->payload);
            }
            return 1;
        }
            /* if user wants to create a chatroom */
        case MULTICAST_CREATE: {
            /* we check if chatroom name is not used yet */
            for (int j = 0; j < NB_CHATROOMS; j++) {
                if (server->chatrooms[j] != NULL) {
                    /* if chatroom name exists yet */
                    if (strcmp(server->chatrooms[j]->name, server->msgStruct.infos) == 0) {
                        strcpy(server->payload, "Chat room name used by other users, please choose a new chatroom name");
                        server->msgStruct.payloadLen = strlen(server->payload);
                        strcpy(server->msgStruct.nickSender, "SERVER");
                        sendMsg(clientFd, &server->msgStruct, server->payload);
                        return 1;
                    }
                }
            }
            /* if chatrooom's name does not exist yet */
            /* we remove the user from all the chatrooms he is in before creating a new chatroom */
            struct1_t arg1 = {server, currentUser};
            chatroom_t **chatroom = getChatroomInServer((void *) &arg1, 0);
            if (chatroom != NULL) {
                user_t **user = getUserInChatroom(chatroom, currentUser);
                *user = NULL;
                (*chatroom)->nbOfUsers--;
                for (int l = 0; l < NB_USERS; l++) {
                    if ((*chatroom)->users[l] != NULL) {
                        sprintf(server->payload, "%s has quit the channel", currentUser->pseudo);
                        server->msgStruct.payloadLen = strlen(server->payload);
                        sprintf(server->msgStruct.nickSender,"SERVER@%s", (*chatroom)->name);
                        sendMsg((*chatroom)->users[l]->socket.fd, &server->msgStruct, server->payload);
                    }
                }
                /* if chatroom is empty, destroying it */
                if ((*chatroom)->nbOfUsers == 0) {
                    sprintf(server->payload, "You were the last user in %s, this channel has been destroyed", (*chatroom)->name);
                    server->msgStruct.payloadLen = strlen(server->payload);
                    strcpy(server->msgStruct.nickSender, "SERVER");
                    sendMsg(clientFd, &server->msgStruct, server->payload);
                    free(*chatroom);
                    *chatroom = NULL;
                }
            }

            /* creating a new chatroom */
            for (int j = 0; j < NB_CHATROOMS; j++) {
                if (server->chatrooms[j] == NULL) {
                    chatroom_t *newChatroom = malloc(sizeof(chatroom_t));
                    memset(newChatroom, 0, sizeof(chatroom_t));
                    strcpy(newChatroom->name, server->msgStruct.infos);
                    newChatroom->users[0] = currentUser;
                    currentUser->inChatroom = 1;
                    newChatroom->nbOfUsers = 1;
                    server->chatrooms[j] = newChatroom;
                    sprintf(server->payload, "You have created channel %s", newChatroom->name);
                    server->msgStruct.payloadLen = strlen(server->payload);
                    strcpy(server->msgStruct.nickSender, "SERVER");
                    sendMsg(clientFd, &server->msgStruct, server->payload);
                    return 1;
                }
            }
            return 1;
        }
            /* if user wants to have the list of all the chatrooms created */
        case MULTICAST_LIST: {
            strcpy(server->payload, "Online chatrooms are:\n");
            for (int j = 0; j < NB_CHATROOMS; j++) {
                if (server->chatrooms[j] != NULL) {
                    sprintf(server->payload + strlen(server->payload), "- %s\n", server->chatrooms[j]->name);
                }
            }
            server->payload[strlen(server->payload) - 1] = '\0';
            server->msgStruct.payloadLen = strlen(server->payload);
            strcpy(server->msgStruct.nickSender, "SERVER");
            sendMsg(clientFd, &server->msgStruct, server->payload);
            return 1;
        }
            /* if user wants to join a chatroom */
        case MULTICAST_JOIN: {
            struct2_t arg1 = {server, server->msgStruct.infos};
            /* getting chatroom user wants to join */
            chatroom_t **chatroom = getChatroomInServer((void *) &arg1, 1);
            /* if chatroom exists */
            if (chatroom != NULL) {
                struct1_t arg = {server, currentUser};
                chatroom_t **userChatroom = getChatroomInServer((void *) &arg, 0);
                /* if user is in a chatroom*/
                if (userChatroom != NULL) {
                    /* if user is yet in the chatroom */
                    if (userChatroom == chatroom) {
                        for (int j = 0; j < NB_USERS; j++) {
                            if ((*chatroom)->users[j] == currentUser) {
                                strcpy(server->payload, "You were yet in this chatroom");
                                server->msgStruct.payloadLen = strlen(server->payload);
                                strcpy(server->msgStruct.nickSender, "SERVER");
                                sendMsg(clientFd, &server->msgStruct, server->payload);
                                return 1;
                            }
                        }
                    } else {
                        user_t **user = getUserInChatroom(userChatroom, currentUser);
                        *user = NULL;
                        (*userChatroom)->nbOfUsers--;
                        for (int l = 0; l < NB_USERS; l++) {
                            if ((*userChatroom)->users[l] != NULL) {
                                sprintf(server->payload, "%s has quit the channel", currentUser->pseudo);
                                server->msgStruct.payloadLen = strlen(server->payload);
                                sprintf(server->msgStruct.nickSender,"SERVER@%s", (*userChatroom)->name);
                                sendMsg((*userChatroom)->users[l]->socket.fd, &server->msgStruct, server->payload);
                            }
                        }
                        /* if chatroom is empty, destroying it */
                        if ((*userChatroom)->nbOfUsers == 0) {
                            sprintf(server->payload, "You were the last user in %s, this channel has been destroyed", (*userChatroom)->name);
                            server->msgStruct.payloadLen = strlen(server->payload);
                            strcpy(server->msgStruct.nickSender, "SERVER");
                            sendMsg(clientFd, &server->msgStruct, server->payload);
                            free(*userChatroom);
                            *userChatroom = NULL;
                        }
                    }
                }
                /* joining chatroom */
                arg1.name = server->msgStruct.infos;
                arg1.server = server;
                chatroom = getChatroomInServer((void *) &arg1, 1);
                int joined = 0;
                for (int k = 0; k < NB_USERS; k++) {
                    if ((*chatroom)->users[k] == NULL && joined == 0) {
                        (*chatroom)->users[k] = currentUser;
                        currentUser->inChatroom = 1;
                        (*chatroom)->nbOfUsers++;
                        sprintf(server->payload, "You have joined %s", server->msgStruct.infos);
                        server->msgStruct.payloadLen = strlen(server->payload);
                        strcpy(server->msgStruct.nickSender, "SERVER");
                        sendMsg(clientFd, &server->msgStruct, server->payload);
                        joined = 1;
                    } else if ((*chatroom)->users[k] != NULL) {
                        /* informing the other users in the chatroom that a new user joined*/
                        sprintf(server->payload, "%s has joined the channel", currentUser->pseudo);
                        server->msgStruct.payloadLen = strlen(server->payload);
                        sprintf(server->msgStruct.nickSender, "SERVER@%s", (*chatroom)->name);
                        sendMsg((*chatroom)->users[k]->socket.fd, &server->msgStruct, server->payload);
                    }
                }
            } else {
                /* if chatroom does not exist */
                sprintf(server->payload, "Channel %s does not exist", server->msgStruct.infos);
                server->msgStruct.payloadLen = strlen(server->payload);
                strcpy(server->msgStruct.nickSender, "SERVER");
                sendMsg(clientFd, &server->msgStruct, server->payload);
                return 1;
            }
            return 1;
        }
            /* if user wants to quit a chatroom */
        case MULTICAST_QUIT: {
            struct2_t arg1 = {server, server->msgStruct.infos};
            /* getting chatroom user wants to quit */
            chatroom_t **chatroom = getChatroomInServer((void *) &arg1, 1);
            if (chatroom != NULL) {
                user_t **user = getUserInChatroom(chatroom, currentUser);
                /* if chatroom exists */
                if (*user != NULL) {
                    /* removing the user from the chatroom */
                    *user = NULL;
                    currentUser->inChatroom = 0;
                    (*chatroom)->nbOfUsers--;
                    sprintf(server->payload, "You have quit the channel %s", (*chatroom)->name);
                    server->msgStruct.payloadLen = strlen(server->payload);
                    strcpy(server->msgStruct.nickSender, "SERVER");
                    sendMsg(clientFd, &server->msgStruct, server->payload);
                    /* if chatroom is now empty */
                    if ((*chatroom)->nbOfUsers == 0) {
                        sprintf(server->payload, "You were the last user in this channel, %s has been destroyed", (*chatroom)->name);
                        server->msgStruct.payloadLen = strlen(server->payload);
                        strcpy(server->msgStruct.nickSender, "SERVER");
                        sendMsg(clientFd, &server->msgStruct, server->payload);
                        free(*chatroom);
                        *chatroom = NULL;
                    } else {
                        for (int l = 0; l < NB_USERS; l++) {
                            if ((*chatroom)->users[l] != NULL) {
                                sprintf(server->payload, "%s has quit the channel", currentUser->pseudo);
                                server->msgStruct.payloadLen = strlen(server->payload);
                                sprintf(server->msgStruct.nickSender, "SERVER@%s", (*chatroom)->name);
                                sendMsg((*chatroom)->users[l]->socket.fd, &server->msgStruct, server->payload);
                            }
                        }
                    }
                } else {
                    /* if user was not in the chatroom */
                    sprintf(server->payload, "You were not in chatroom %s", (*chatroom)->name);
                    server->msgStruct.payloadLen = strlen(server->payload);
                    strcpy(server->msgStruct.nickSender, "SERVER");
                    sendMsg(clientFd, &server->msgStruct, server->payload);
                }
            } else {
                /* if chatroom does not exist */
                sprintf(server->payload, "Channel %s does not exist", server->msgStruct.infos);
                server->msgStruct.payloadLen = strlen(server->payload);
                strcpy(server->msgStruct.nickSender, "SERVER");
                sendMsg(clientFd, &server->msgStruct, server->payload);
            }
            return 1;
        }
            /* if user wants to send a message in the chatroom */
        case MULTICAST_SEND:
            /* if user is not in a chatroom*/
            if (currentUser->inChatroom == 0) {
                /* message type is ECHO_SEND*/
                server->msgStruct.type = ECHO_SEND;
                strcpy(server->msgStruct.nickSender, "SERVER");
                strcpy(server->payload, server->buffer);
                sendMsg(clientFd, &server->msgStruct, server->payload);
                return 1;
            } else {
                struct1_t arg1 = {server, currentUser};
                /* getting the chatroom the user wants to send the message in */
                chatroom_t **chatroom = getChatroomInServer((void *) &arg1, 0);
                /* sending message to all the users in the chatroom */
                for (int l = 0; l < NB_USERS; l++) {
                    if ((*chatroom)->users[l] != NULL) {
                        if (strcmp((*chatroom)->users[l]->pseudo, currentUser->pseudo) == 0) {
                            sprintf(server->msgStruct.nickSender, "me@%s", (*chatroom)->name);
                        } else {
                            sprintf(server->msgStruct.nickSender, "%s@%s", currentUser->pseudo, (*chatroom)->name);
                        }
                        strcpy(server->payload, server->buffer);
                        sendMsg((*chatroom)->users[l]->socket.fd, &server->msgStruct, server->payload);
                    }
                }
                return 1;
            }
            /* if user wants to send a file to another user */
        case FILE_REQUEST: {
            /* getting the user he wants to send file to */
            user_t *dstUser = getUser(server, server->msgStruct.infos);
            /* if user does not exist */
            if (dstUser == NULL) {
                server->msgStruct.type = DEFAULT;
                sprintf(server->payload, "%s does not exist", server->msgStruct.infos);
                server->msgStruct.payloadLen = strlen(server->payload);
                strcpy(server->msgStruct.nickSender, "SERVER");
                sendMsg(clientFd, &server->msgStruct, server->payload);
            } else if (dstUser == currentUser) {
                server->msgStruct.type = DEFAULT;
                strcpy(server->payload, "You cannot send a file to yourself");
                server->msgStruct.payloadLen = strlen(server->payload);
                strcpy(server->msgStruct.nickSender, "SERVER");
                sendMsg(clientFd, &server->msgStruct, server->payload);
            } else {
                strcpy(server->payload, server->buffer);
                strcpy(server->msgStruct.infos, server->msgStruct.nickSender);
                server->msgStruct.payloadLen = strlen(server->payload);
                strcpy(server->msgStruct.nickSender, "SERVER");
                sendMsg(dstUser->socket.fd, &server->msgStruct, server->payload);
            }
            return 1;
        }
            /* if user accept file transfer */
        case FILE_ACCEPT: {
            user_t *dstUser = getUser(server, server->msgStruct.infos);
            strcpy(server->payload, server->buffer);
            server->msgStruct.payloadLen = strlen(server->payload);
            sendMsg(dstUser->socket.fd, &server->msgStruct, server->payload);
            return 1;
        }
            /* if user rejects file transfer */
        case FILE_REJECT: {
            user_t *dstUser = getUser(server, server->msgStruct.infos);
            sprintf(server->payload, "%s cancelled file transfer", currentUser->pseudo);
            server->msgStruct.payloadLen = strlen(server->payload);
            strcpy(server->msgStruct.nickSender, "SERVER");
            sendMsg(dstUser->socket.fd, &server->msgStruct, server->payload);
            return 1;
        }
        default:
            return 1;
    }
}

_Noreturn void runServer(struct server *server) {
    /* Declare array of struct pollfd */
    struct pollfd pollfds[NB_USERS];

    /* Init first slot with listening socket */
    pollfds[0].fd = server->socket.fd;
    pollfds[0].events = POLLIN;
    pollfds[0].revents = 0;
    /* Init remaining slot to default values */
    for (int i = 1; i < NB_USERS; i++) {
        pollfds[i].fd = -1;
        pollfds[i].events = 0;
        pollfds[i].revents = 0;
    }
    /* server loop */
    while (1) {
        /* Block until new activity detected on existing sockets */
        int nbActiveSockets = 0;
        if (-1 == (nbActiveSockets = poll(pollfds, NB_USERS, -1))) {
            perror("Poll");
        }
        printf("[SERVER] : %d active socket\n", nbActiveSockets);
        /* Iterate on the array of monitored struct pollfd */
        for (int i = 0; i < NB_USERS; i++) {
            /* If listening socket is active => accept new incoming connection */
            if (pollfds[i].fd == server->socket.fd && pollfds[i].revents & POLLIN) {
                /* accept new connection and retrieve new socket file descriptor */
                struct sockaddr clientAddr;
                socklen_t size = sizeof(clientAddr);
                struct socket socket;
                if (-1 == (socket.fd = accept(server->socket.fd, &clientAddr, &size))) {
                    perror("Accept");
                }
                /* getting ip address and port number of the new connection */
                getpeername(socket.fd, &clientAddr, &size);
                struct sockaddr_in *sockaddrInPtr = (struct sockaddr_in *)&clientAddr;
                socket.port = ntohs(sockaddrInPtr->sin_port);
                strcpy(socket.ipAddr, inet_ntoa(sockaddrInPtr->sin_addr));
                printf("Client (%s:%hu) connected.\n", socket.ipAddr, socket.port);
                /* adding a new user */
                char pseudo[NICK_LEN] = "";
                time_t ltime;
                time(&ltime);
                struct tm *newtime = localtime(&ltime);
                char date[MSG_LEN];
                strcpy(date, asctime(newtime));
                pushUser(server, &socket, pseudo, date, 0);
                /* store new file descriptor in available slot in the array of struct pollfd set .events to POLLIN */
                for (int j = 0; j < NB_USERS; j++) {
                    if (pollfds[j].fd == -1) {
                        pollfds[j].fd = socket.fd;
                        pollfds[j].events = POLLIN;
                        break;
                    }
                }
                /* Set .revents of listening socket back to default */
                pollfds[i].revents = 0;
            } else if (pollfds[i].fd != server->socket.fd && pollfds[i].revents & POLLHUP) {
                /* If a socket previously created to communicate with a client detects a disconnection from the client side */
                printf("client on socket %d has disconnected from server\n", pollfds[i].fd);
                /* freeing disconnected user */
                freeUser(server, pollfds[i].fd);
                /* Close socket and set struct pollfd back to default */
                close(pollfds[i].fd);
                pollfds[i].fd = -1;
                pollfds[i].events = 0;
                pollfds[i].revents = 0;
            } else if (pollfds[i].fd != server->socket.fd && pollfds[i].revents & POLLIN) {
                /* If a socket different from the listening socket is active */
                /* Processing user request */
                if (!processRequest(pollfds[i].fd, server)) {
                    close(pollfds[i].fd);
                    pollfds[i].fd = -1;
                    pollfds[i].events = 0;
                    pollfds[i].revents = 0;
                }
                /* set back .revents to 0 */
                pollfds[i].revents = 0;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./server portNumber\n");
        exit(EXIT_FAILURE);
    }
    char portNumber[INFOS_LEN];
    strcpy(portNumber, argv[1]);
    /* creating server socket */
    struct server *server = malloc(sizeof(struct server));
    memset(server, 0, sizeof(struct server));
    server->socket = socketAndBind(portNumber);
    if ((listen(server->socket.fd, SOMAXCONN)) != 0) {
        free(server);
        perror("listen()");
        exit(EXIT_FAILURE);
    }
    printf("Listening on %s:%hu\n", server->socket.ipAddr, server->socket.port);
    /* running server */
    runServer(server);
}