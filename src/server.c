#include "../include/server.h"
#include <poll.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>

/* adding user in the list */
void pushUser(struct server *server, struct user *user) {
    user->next = server->users;
    server->users = user;
}

/* freeing user node in the list */
void freeUser(struct server *server) {
    user_t *newNode = malloc(sizeof(user_t));
    newNode->next = server->users;
    server->users = newNode;
    user_t *current;
    /* looking for the user we want to free */
    for (current = server->users; current->next != NULL; current = current->next) {
        if (current->next == server->currentUser) {
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
    struct user *user;
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
        struct user *user = ptr->user;
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

void nicknameNew(struct server *server) {
    strcpy(server->packet.header.nickSender, "SERVER");
    for (user_t *user = server->users; user != NULL; user = user->next) {
        /* looking if pseudo used by another user */
        if (strcmp(user->pseudo, server->packet.header.infos) == 0) {
            strcpy(server->packet.payload, "Pseudo used by another user");
            server->packet.header.payloadLen = strlen(server->packet.payload);
            server->packet.header.type = DEFAULT;
            sendPacket(server->currentUser->socket.fd, &server->packet);
            return;
        }
    }
    /* if pseudo not used yet, adding user pseudo */
    strcpy(server->currentUser->pseudo, server->packet.header.infos);
    sprintf(server->packet.payload, "Welcome on the chat %s, type a command or type /help if you need help", server->currentUser->pseudo);
    server->packet.header.payloadLen = strlen(server->packet.payload);
    sendPacket(server->currentUser->socket.fd, &server->packet);
}

void nicknameList(struct server *server) {
    strcpy(server->packet.header.nickSender, "SERVER");
    strcpy(server->packet.payload, "Online users are:\n");
    for (user_t *user = server->users; user != NULL; user = user->next) {
        sprintf(server->packet.payload + strlen(server->packet.payload), "- %s\n", user->pseudo);
    }
    server->packet.payload[strlen(server->packet.payload) - 1] = '\0';
    server->packet.header.payloadLen = strlen(server->packet.payload);
    sendPacket(server->currentUser->socket.fd, &server->packet);
}

void nicknameInfos(struct server *server) {
    strcpy(server->packet.header.nickSender, "SERVER");
    /* getting user we want information of */
    user_t *dstUser = getUser(server, server->packet.header.infos);
    /* if user does not exist */
    if (dstUser == NULL) {
        strcpy(server->packet.payload, "Unknown user");
        server->packet.header.payloadLen = strlen(server->packet.payload);
    } else { /* if user exists */
        sprintf(server->packet.payload, "%s is connected since %s with IP address %s and port number %hu", dstUser->pseudo, dstUser->date, dstUser->socket.ipAddr, dstUser->socket.port);
        server->packet.header.payloadLen = strlen(server->packet.payload);
    }
    sendPacket(server->currentUser->socket.fd, &server->packet);
}

void broadcastSend(struct server *server) {
    for (user_t *dstUser = server->users; dstUser != NULL; dstUser = dstUser->next) {
        if (strcmp(dstUser->pseudo, server->currentUser->pseudo) == 0) {
            strcpy(server->packet.header.nickSender, "me@all");
        } else {
            sprintf(server->packet.header.nickSender, "%s@all", server->currentUser->pseudo);
        }
        sendPacket(dstUser->socket.fd, &server->packet);
    }
}

void unicastSend(struct server *server) {
    user_t *dstUser = getUser(server, server->packet.header.infos);
    /* if dest user exists */
    if (dstUser != NULL) {
        sendPacket(dstUser->socket.fd, &server->packet);
        sprintf(server->packet.header.nickSender, "me@%s", dstUser->pseudo);
        sendPacket(server->currentUser->socket.fd, &server->packet);
    } else {
        /* if dest user does not exist */
        sprintf(server->packet.payload, "User %s does not exist", server->packet.header.infos);
        strcpy(server->packet.header.nickSender, "SERVER");
        server->packet.header.payloadLen = strlen(server->packet.payload);
        sendPacket(server->currentUser->socket.fd, &server->packet);
    }
}

void multicastCreate(struct server *server) {
    /* we check if chatroom name is not used yet */
    for (int j = 0; j < NB_CHATROOMS; j++) {
        if (server->chatrooms[j] != NULL) {
            /* if chatroom name exists yet */
            if (strcmp(server->chatrooms[j]->name, server->packet.header.infos) == 0) {
                strcpy(server->packet.payload, "Chat room name used by other users, please choose a new chatroom name");
                server->packet.header.payloadLen = strlen(server->packet.payload);
                strcpy(server->packet.header.nickSender, "SERVER");
                sendPacket(server->currentUser->socket.fd, &server->packet);
                return;
            }
        }
    }
    /* if chatrooom's name does not exist yet */
    /* we remove the user from all the chatrooms he is in before creating a new chatroom */
    struct1_t arg1 = {server, server->currentUser};
    chatroom_t **chatroom = getChatroomInServer((void *) &arg1, 0);
    if (chatroom != NULL) {
        user_t **user = getUserInChatroom(chatroom, server->currentUser);
        *user = NULL;
        (*chatroom)->nbOfUsers--;
        for (int l = 0; l < NB_USERS; l++) {
            if ((*chatroom)->users[l] != NULL) {
                sprintf(server->packet.payload, "%s has quitted the channel", server->currentUser->pseudo);
                server->packet.header.payloadLen = strlen(server->packet.payload);
                sprintf(server->packet.header.nickSender, "SERVER@%s", (*chatroom)->name);
                sendPacket((*chatroom)->users[l]->socket.fd, &server->packet);
            }
        }
        /* if chatroom is empty, destroying it */
        if ((*chatroom)->nbOfUsers == 0) {
            sprintf(server->packet.payload, "You were the last user in %s, this channel has been destroyed", (*chatroom)->name);
            server->packet.header.payloadLen = strlen(server->packet.payload);
            strcpy(server->packet.header.nickSender, "SERVER");
            sendPacket(server->currentUser->socket.fd, &server->packet);
            free(*chatroom);
            *chatroom = NULL;
        }
    }
    /* creating a new chatroom */
    for (int j = 0; j < NB_CHATROOMS; j++) {
        if (server->chatrooms[j] == NULL) {
            chatroom_t *newChatroom = malloc(sizeof(chatroom_t));
            memset(newChatroom, 0, sizeof(chatroom_t));
            strcpy(newChatroom->name, server->packet.header.infos);
            newChatroom->users[0] = server->currentUser;
            server->currentUser->inChatroom = 1;
            newChatroom->nbOfUsers = 1;
            server->chatrooms[j] = newChatroom;
            sprintf(server->packet.payload, "You have created channel %s", newChatroom->name);
            server->packet.header.payloadLen = strlen(server->packet.payload);
            strcpy(server->packet.header.nickSender, "SERVER");
            sendPacket(server->currentUser->socket.fd, &server->packet);
            return;
        }
    }
}

void multicastList(struct server *server) {
    strcpy(server->packet.payload, "Online chatrooms are:\n");
    for (int j = 0; j < NB_CHATROOMS; j++) {
        if (server->chatrooms[j] != NULL) {
            sprintf(server->packet.payload + strlen(server->packet.payload), "- %s\n", server->chatrooms[j]->name);
        }
    }
    server->packet.payload[strlen(server->packet.payload) - 1] = '\0';
    server->packet.header.payloadLen = strlen(server->packet.payload);
    strcpy(server->packet.header.nickSender, "SERVER");
    sendPacket(server->currentUser->socket.fd, &server->packet);
}

void multicastJoin(struct server *server) {
    struct2_t arg1 = {server, server->packet.header.infos};
    /* getting chatroom user wants to join */
    chatroom_t **chatroomToJoin = getChatroomInServer((void *) &arg1, 1);
    /* if chatroom exists */
    if (chatroomToJoin != NULL) {
        struct1_t arg1 = {server, server->currentUser};
        chatroom_t **currentChatroom = getChatroomInServer((void *) &arg1, 0);
        /* if user is in a chatroom*/
        if (currentChatroom != NULL) {
            /* if user is yet in the chatroom */
            if (currentChatroom == chatroomToJoin) {
                for (int j = 0; j < NB_USERS; j++) {
                    if ((*chatroomToJoin)->users[j] == server->currentUser) {
                        strcpy(server->packet.payload, "You were yet in this chatroom");
                        server->packet.header.payloadLen = strlen(server->packet.payload);
                        strcpy(server->packet.header.nickSender, "SERVER");
                        sendPacket(server->currentUser->socket.fd, &server->packet);
                        return;
                    }
                }
            } else {
                user_t **user = getUserInChatroom(currentChatroom, server->currentUser);
                *user = NULL;
                (*currentChatroom)->nbOfUsers--;
                for (int l = 0; l < NB_USERS; l++) {
                    if ((*currentChatroom)->users[l] != NULL) {
                        sprintf(server->packet.payload, "%s has quitted the channel", server->currentUser->pseudo);
                        server->packet.header.payloadLen = strlen(server->packet.payload);
                        sprintf(server->packet.header.nickSender, "SERVER@%s", (*currentChatroom)->name);
                        sendPacket((*currentChatroom)->users[l]->socket.fd, &server->packet);
                    }
                }
                /* if chatroom is empty, destroying it */
                if ((*currentChatroom)->nbOfUsers == 0) {
                    sprintf(server->packet.payload, "You were the last user in %s, this channel has been destroyed", (*currentChatroom)->name);
                    server->packet.header.payloadLen = strlen(server->packet.payload);
                    strcpy(server->packet.header.nickSender, "SERVER");
                    sendPacket(server->currentUser->socket.fd, &server->packet);
                    free(*currentChatroom);
                    *currentChatroom = NULL;
                }
            }
        }
        /* joining chatroom */
        int joined = 0;
        for (int k = 0; k < NB_USERS; k++) {
            if ((*chatroomToJoin)->users[k] == NULL && joined == 0) {
                (*chatroomToJoin)->users[k] = server->currentUser;
                server->currentUser->inChatroom = 1;
                (*chatroomToJoin)->nbOfUsers++;
                sprintf(server->packet.payload, "You have joined %s", server->packet.header.infos);
                server->packet.header.payloadLen = strlen(server->packet.payload);
                strcpy(server->packet.header.nickSender, "SERVER");
                sendPacket(server->currentUser->socket.fd, &server->packet);
                joined = 1;
            } else if ((*chatroomToJoin)->users[k] != NULL) {
                /* informing the other users in the chatroom that a new user joined*/
                sprintf(server->packet.payload, "%s has joined the channel", server->currentUser->pseudo);
                server->packet.header.payloadLen = strlen(server->packet.payload);
                sprintf(server->packet.header.nickSender, "SERVER@%s", (*chatroomToJoin)->name);
                sendPacket((*chatroomToJoin)->users[k]->socket.fd, &server->packet);
            }
        }
    } else {
        /* if chatroom does not exist */
        sprintf(server->packet.payload, "Channel %s does not exist", server->packet.header.infos);
        server->packet.header.payloadLen = strlen(server->packet.payload);
        strcpy(server->packet.header.nickSender, "SERVER");
        sendPacket(server->currentUser->socket.fd, &server->packet);
        return;
    }
}

void multicastQuit(struct server *server) {
    struct2_t arg1 = {server, server->packet.header.infos};
    /* getting chatroom user wants to quit */
    chatroom_t **chatroom = getChatroomInServer((void *) &arg1, 1);
    if (chatroom != NULL) {
        user_t **user = getUserInChatroom(chatroom, server->currentUser);
        /* if chatroom exists */
        if (*user != NULL) {
            /* removing the user from the chatroom */
            *user = NULL;
            server->currentUser->inChatroom = 0;
            (*chatroom)->nbOfUsers--;
            sprintf(server->packet.payload, "You have quitted the channel %s", (*chatroom)->name);
            server->packet.header.payloadLen = strlen(server->packet.payload);
            strcpy(server->packet.header.nickSender, "SERVER");
            sendPacket(server->currentUser->socket.fd, &server->packet);
            /* if chatroom is now empty */
            if ((*chatroom)->nbOfUsers == 0) {
                sprintf(server->packet.payload, "You were the last user in this channel, %s has been destroyed", (*chatroom)->name);
                server->packet.header.payloadLen = strlen(server->packet.payload);
                strcpy(server->packet.header.nickSender, "SERVER");
                sendPacket(server->currentUser->socket.fd, &server->packet);
                free(*chatroom);
                *chatroom = NULL;
            } else {
                for (int l = 0; l < NB_USERS; l++) {
                    if ((*chatroom)->users[l] != NULL) {
                        sprintf(server->packet.payload, "%s has quitted the channel", server->currentUser->pseudo);
                        server->packet.header.payloadLen = strlen(server->packet.payload);
                        sprintf(server->packet.header.nickSender, "SERVER@%s", (*chatroom)->name);
                        sendPacket((*chatroom)->users[l]->socket.fd, &server->packet);
                    }
                }
            }
        } else {
            /* if user was not in the chatroom */
            sprintf(server->packet.payload, "You were not in chatroom %s", (*chatroom)->name);
            server->packet.header.payloadLen = strlen(server->packet.payload);
            strcpy(server->packet.header.nickSender, "SERVER");
            sendPacket(server->currentUser->socket.fd, &server->packet);
        }
    } else {
        /* if chatroom does not exist */
        sprintf(server->packet.payload, "Channel %s does not exist", server->packet.header.infos);
        server->packet.header.payloadLen = strlen(server->packet.payload);
        strcpy(server->packet.header.nickSender, "SERVER");
        sendPacket(server->currentUser->socket.fd, &server->packet);
    }
}

void multicastSend(struct server *server) {
    /* if user is not in a chatroom*/
    if (server->currentUser->inChatroom == 0) {
        /* message type is ECHO_SEND*/
        server->packet.header.type = ECHO_SEND;
        strcpy(server->packet.header.nickSender, "SERVER");
        sendPacket(server->currentUser->socket.fd, &server->packet);
    } else {
        /* getting the chatroom the user wants to send the message in */
        struct1_t arg1 = {server, server->currentUser};
        chatroom_t **chatroom = getChatroomInServer((void *) &arg1, 0);
        /* sending message to all the users in the chatroom */
        for (int l = 0; l < NB_USERS; l++) {
            if ((*chatroom)->users[l] != NULL) {
                if (strcmp((*chatroom)->users[l]->pseudo, server->currentUser->pseudo) == 0) {
                    sprintf(server->packet.header.nickSender, "me@%s", (*chatroom)->name);
                } else {
                    sprintf(server->packet.header.nickSender, "%s@%s", server->currentUser->pseudo, (*chatroom)->name);
                }
                sendPacket((*chatroom)->users[l]->socket.fd, &server->packet);
            }
        }
    }
}

void fileRequest(struct server *server) {
    /* getting the user he wants to send file to */
    user_t *dstUser = getUser(server, server->packet.header.infos);
    /* if user does not exist */
    if (dstUser == NULL) {
        server->packet.header.type = DEFAULT;
        sprintf(server->packet.payload, "%s does not exist", server->packet.header.infos);
        server->packet.header.payloadLen = strlen(server->packet.payload);
        strcpy(server->packet.header.nickSender, "SERVER");
        sendPacket(server->currentUser->socket.fd, &server->packet);
    } else if (dstUser == server->currentUser) {
        server->packet.header.type = DEFAULT;
        strcpy(server->packet.payload, "You cannot send a file to yourself");
        server->packet.header.payloadLen = strlen(server->packet.payload);
        strcpy(server->packet.header.nickSender, "SERVER");
        sendPacket(server->currentUser->socket.fd, &server->packet);
    } else {
        strcpy(server->packet.header.infos, server->packet.header.nickSender);
        server->packet.header.payloadLen = strlen(server->packet.payload);
        strcpy(server->packet.header.nickSender, "SERVER");
        sendPacket(dstUser->socket.fd, &server->packet);
    }
}

void fileAccept(struct server *server) {
    user_t *dstUser = getUser(server, server->packet.header.infos);
    sendPacket(dstUser->socket.fd, &server->packet);
}

void fileReject(struct server *server) {
    user_t *dstUser = getUser(server, server->packet.header.infos);
    sprintf(server->packet.payload, "%s cancelled file transfer", server->currentUser->pseudo);
    server->packet.header.payloadLen = strlen(server->packet.payload);
    strcpy(server->packet.header.nickSender, "SERVER");
    sendPacket(dstUser->socket.fd, &server->packet);
}

int processRequest(struct server *server) {

    /* Cleaning memory */
    memset(&server->packet.header, 0, sizeof(struct header));
    memset(server->packet.payload, 0, MSG_LEN);
    /* Receiving structure */
    /* if client disconnected */
    if ((recv(server->currentUser->socket.fd, &server->packet.header, sizeof(struct header), MSG_WAITALL)) <= 0) {
        return 0;
    }
    /* Receiving message */
    memset(server->packet.payload, 0, MSG_LEN);
    if (server->packet.header.payloadLen != 0 && recv(server->currentUser->socket.fd, server->packet.payload, server->packet.header.payloadLen, MSG_WAITALL) <= 0) {
        perror("recv");
        return 1;
    }
    printf("payloadLen: %ld / nickSender: %s / type: %s / infos: %s\n", server->packet.header.payloadLen, server->packet.header.nickSender, msgTypeStr[server->packet.header.type], server->packet.header.infos);
    printf("payload: %s\n", server->packet.payload);

    switch (server->packet.header.type) {
        /* if user wants to change/create pseudo */
        case NICKNAME_NEW:
            nicknameNew(server);
            return 1;
            /* if user wants to see the list of other connected users */
        case NICKNAME_LIST:
            nicknameList(server);
            return 1;
            /* if user wants to know the ip address, remote port number and connection date of another user */
        case NICKNAME_INFOS:
            nicknameInfos(server);
            return 1;
            /* if user wants to send a message to all the users */
        case BROADCAST_SEND:
            broadcastSend(server);
            return 1;
            /* if user wants to send a message to another specific user */
        case UNICAST_SEND:
            unicastSend(server);
            return 1;
            /* if user wants to create a chatroom */
        case MULTICAST_CREATE:
            multicastCreate(server);
            return 1;
            /* if user wants to have the list of all the chatrooms created */
        case MULTICAST_LIST:
            multicastList(server);
            return 1;
            /* if user wants to join a chatroom */
        case MULTICAST_JOIN:
            multicastJoin(server);
            return 1;
            /* if user wants to quit a chatroom */
        case MULTICAST_QUIT:
            multicastQuit(server);
            return 1;
            /* if user wants to send a message in the chatroom */
        case MULTICAST_SEND:
            multicastSend(server);
            return 1;
            /* if user wants to send a file to another user */
        case FILE_REQUEST:
            fileRequest(server);
            return 1;
            /* if user accept file transfer */
        case FILE_ACCEPT:
            fileAccept(server);
            return 1;
            /* if user rejects file transfer */
        case FILE_REJECT:
            fileReject(server);
            return 1;
        default:
            return 1;
    }
}

void disconnectUser(struct server *server, struct user *user) {
    printf("%s (%s:%hu) on socket %d has disconnected from server\n", user->pseudo, user->socket.ipAddr, user->socket.port, user->socket.fd);
    /* if user is in a chatroom
       we remove the user from the chatroom */
    if (user->inChatroom == 1) {
        /* getting the chatroom in which the user is */
        struct1_t arg1 = {server, server->currentUser};
        chatroom_t **chatroom = getChatroomInServer((void *) &arg1, 0);
        /* getting the user in the chatroom */
        user_t **user = getUserInChatroom(chatroom, server->currentUser);
        /* removing the user in the chatroom */
        *user = NULL;
        (*chatroom)->nbOfUsers--;
        /* informing all the users in the chatroom that a user has quitted the chatroom */
        for (int l = 0; l < NB_USERS; l++) {
            if ((*chatroom)->users[l] != NULL) {
                sprintf(server->packet.payload, "%s has quitted the channel", server->currentUser->pseudo);
                server->packet.header.payloadLen = strlen(server->packet.payload);
                strcpy(server->packet.header.nickSender, "SERVER");
                sendPacket((*chatroom)->users[l]->socket.fd, &server->packet);
            }
        }
        /* deleting the chatroom if chatroom is now empty */
        if ((*chatroom)->nbOfUsers == 0) {
            free(*chatroom);
            *chatroom = NULL;
        }
    }
    /* Close socket and set struct pollfd back to default */
    freeUser(server);
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
                struct sockaddr_in *sockaddrInPtr = (struct sockaddr_in *) &clientAddr;
                socket.port = ntohs(sockaddrInPtr->sin_port);
                strcpy(socket.ipAddr, inet_ntoa(sockaddrInPtr->sin_addr));
                printf("Client (%s:%hu) connected on socket %i.\n", socket.ipAddr, socket.port, socket.fd);
                /* adding a new user */
                char pseudo[NICK_LEN] = "";
                time_t ltime;
                time(&ltime);
                struct tm *newtime = localtime(&ltime);
                char date[MSG_LEN];
                strcpy(date, asctime(newtime));
                struct user *user = malloc(sizeof(struct user));
                memcpy(&user->socket, &socket, sizeof(struct socket));
                strcpy(user->pseudo, pseudo);
                strcpy(user->date, date);
                user->inChatroom = 0;
                pushUser(server, user);
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
                /* getting the user which is doing the request */
                struct user *current;
                for (current = server->users; current != NULL; current = current->next) {
                    if (current->socket.fd == pollfds[i].fd) {
                        break;
                    }
                }
                /* If a socket previously created to communicate with a client detects a disconnection from the client side */
                disconnectUser(server, current);
                /* Close socket and set struct pollfd back to default */
                close(pollfds[i].fd);
                pollfds[i].fd = -1;
                pollfds[i].events = 0;
                pollfds[i].revents = 0;
            } else if (pollfds[i].fd != server->socket.fd && pollfds[i].revents & POLLIN) {
                /* If a socket different from the listening socket is active */
                /* getting the user which is doing the request */
                struct user *current;
                for (current = server->users; current != NULL; current = current->next) {
                    if (current->socket.fd == pollfds[i].fd) {
                        break;
                    }
                }
                server->currentUser = current;
                /* Processing user request */
                if (!processRequest(server)) {
                    disconnectUser(server, server->currentUser);
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

struct server *serverInit(char *port) {
    /* creating server socket */
    struct server *server = malloc(sizeof(struct server));
    memset(server, 0, sizeof(struct server));
    server->socket = socketAndBind(port);
    if ((listen(server->socket.fd, SOMAXCONN)) != 0) {
        free(server);
        perror("listen()");
        exit(EXIT_FAILURE);
    }
    printf("Listening on %s:%hu\n", server->socket.ipAddr, server->socket.port);
    return server;
}

void usage() {
    printf("Usage: ./server portNumber\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage();
    }

    struct server *server = serverInit(argv[1]);
    /* running server */
    runServer(server);
}