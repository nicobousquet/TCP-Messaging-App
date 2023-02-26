#include "../include/client.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* copying information in msgstruct */
void fillMsg(struct client *client, char *nickSender, enum msgType type, char *infos, char *payload) {
    if (nickSender != NULL) {
        strcpy(client->msgStruct.nickSender, nickSender);
    }
    client->msgStruct.type = type;
    if (infos != NULL) {
        strcpy(client->msgStruct.infos, infos);
    }
    if (payload != NULL) {
        strcpy(client->payload, payload);
    }
    client->msgStruct.payloadLen = strlen(client->payload);
}

/* Connecting to server socket */
struct socket socketAndConnect(char *hostname, char *port) {
    struct socket sock;
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(hostname, port, &hints, &result) != 0) {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sock.fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock.fd == -1) {
            continue;
        }
        if (connect(sock.fd, rp->ai_addr, rp->ai_addrlen) != -1) {
            /* getting ip address and port of connection */
            struct sockaddr_in *sockAddrInPtr = (struct sockaddr_in *) rp->ai_addr;
            socklen_t len = sizeof(struct sockaddr_in);
            getsockname(sock.fd, (struct sockaddr *) sockAddrInPtr, &len);
            sock.port = ntohs(sockAddrInPtr->sin_port);
            strcpy(sock.ipAddr, inet_ntoa(sockAddrInPtr->sin_addr));
            break;
        }
        close(sock.fd);
    }
    if (rp == NULL) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(result);
    return sock;
}

/* returns type of message depending on command entered by the user */
enum msgType getMsgType(char *firstWord) {
    const char *commands[] = {
            "/nick",
            "/help",
            "/who",
            "/whois",
            "/msgall",
            "/msg",
            "/create",
            "/channel_list",
            "/join",
            "/quit",
            "/send",
            "Y",
            "y",
            "N",
            "n"
    };
    const enum msgType types[] = {
            NICKNAME_NEW,
            HELP,
            NICKNAME_LIST,
            NICKNAME_INFOS,
            BROADCAST_SEND,
            UNICAST_SEND,
            MULTICAST_CREATE,
            MULTICAST_LIST,
            MULTICAST_JOIN,
            QUIT,
            FILE_REQUEST,
            FILE_ACCEPT,
            FILE_ACCEPT,
            FILE_REJECT,
            FILE_REJECT
    };
    for (int i = 0; i < (int) (sizeof(types) / sizeof(enum msgType)); i++) {
        if (strcmp(firstWord, commands[i]) == 0) {
            return types[i];
        }
    }
    return DEFAULT;
}

void help() {
    printf("            ****** HELP ******\n");
    printf("- /nick <pseudo> to change pseudo\n");
    printf("- /whois <pseudo> to have informations on user <pseudo>\n");
    printf("- /who to have the list of online users\n");
    printf("- /msgall to send a message to all user\n");
    printf("- /msg <pseudo> to send a message to user <pseudo>\n");
    printf("- /create <channelName> to create a chat group <channelName>\n");
    printf("- /channel_list to have the list of all online chat groups\n");
    printf("- /join <channelName> to join a chat group <channelName>\n");
    printf("- /quit <channelName> to quit chat group <channelName>\n");
    printf("- /quit to disconnect\n");
    printf("- /send <pseudo> <\"filename\"> to send a file <filename> to user <pseudo>\n");
}

/* disconnecting client from server */
void disconnectFromServer(struct pollfd *pollfds) {
    for (int j = 0; j < NFDS; j++) {
        close(pollfds[j].fd);
        pollfds[j].fd = -1;
        pollfds[j].events = 0;
        pollfds[j].revents = 0;
    }
    printf("You are disconnected\n");
}

/* extracting name of file to send */
void extractFilename(char *tmp, char *subPath) {
    tmp = strtok(tmp, "/");
    strcpy(subPath, tmp);
    /* extracting the name of the file from the whole path */
    while ((tmp = strtok(NULL, "/")) != NULL) {
        memset(subPath, 0, NICK_LEN);
        strcpy(subPath, tmp);
    }
}

/* creation of a server socket */
struct socket bindAndListen(char *listeningPort) {
    struct socket serverSocket = socketAndBind(listeningPort);
    if ((listen(serverSocket.fd, SOMAXCONN)) != 0) {
        perror("listen()\n");
        exit(EXIT_FAILURE);
    }
    printf("Listening on %s:%hu\n", serverSocket.ipAddr, serverSocket.port);
    return serverSocket;
}

/* when receiving a file */
/* writing data from the buffer into a new file */
void downloadingFile(struct clientP2P *serverP2P) {
    /* receiving data of the file from client and saving it into a new file on our computer */
    /* creating directory where the received file will be stored */
    if (mkdir("inbox", S_IRWXU | S_IRWXG | S_IRWXO) && errno != EEXIST) {
        perror("mkdir:");
    }
    char filenameDst[MSG_LEN];
    /* creating the path of the received file */
    sprintf(filenameDst, "inbox/%s", serverP2P->fileStruct.fileToReceive);
    /* opening the file */
    int newFileFd = open(filenameDst, O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
    if (newFileFd == -1) {
        perror("creating file");
        exit(EXIT_FAILURE);
    }
    long size = strtol(serverP2P->buffer, NULL, 10);
    long ret = 0, offset = 0;
    /* while we did not receive all the data of the file */
    while (offset != size) {
        recv(serverP2P->socket.fd, &serverP2P->msgStruct, sizeof(struct message), MSG_WAITALL);
        /* receiving file by frame of max 1024 bytes */
        recv(serverP2P->socket.fd, serverP2P->buffer, serverP2P->msgStruct.payloadLen, MSG_WAITALL);
        /* writing received data in a new file */
        if (-1 == (ret = write(newFileFd, serverP2P->buffer, serverP2P->msgStruct.payloadLen))) {
            perror("Writing in new file");
        }
        offset += ret;
        /* progress bar to show the advancement of downloading */
        char progressBar[12] = {'[', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', ']'};
        int progression = (int) (((double) offset / (double) size) * 100);
        for (int j = 1; j <= progression / 10; j++) {
            progressBar[j] = '#';
        }
        for (int j = 0; j < (int) (sizeof(progressBar) / sizeof(char)); j++) {
            printf("%c", progressBar[j]);
        }
        printf(" [%i%%]\n", progression);
        printf("\033[1A\33[2K\r");
    }
    printf("File received successfully and saved in %s\n", filenameDst);
}

int sendFile(struct clientP2P *clientP2P) {
    struct stat sb;
    stat(clientP2P->fileStruct.fileToSend, &sb);
    if (!S_ISREG(sb.st_mode)) {
        printf("Invalid filename\n");
        clientP2P->msgStruct.type = FILENAME;
        send(clientP2P->socket.fd, &clientP2P->msgStruct, sizeof(struct message), 0);
        close(clientP2P->socket.fd);
        printf("Connection closed with %s\n", clientP2P->fileStruct.nickReceiver);
        return 0;
    }
    /* opening file to send */
    int fileFd = open(clientP2P->fileStruct.fileToSend, O_RDONLY);
    if (fileFd == -1) {
        if (errno == ENOENT) {
            printf("Invalid filename\n");
            clientP2P->msgStruct.type = FILENAME;
            send(clientP2P->socket.fd, &clientP2P->msgStruct, sizeof(struct message), 0);
            close(clientP2P->socket.fd);
            printf("Connection closed with %s\n", clientP2P->fileStruct.nickReceiver);
            return 0;
        }
        perror("Error opening file");
        exit(EXIT_FAILURE);

    }
    /* getting the size of the file to send */
    long size = sb.st_size;
    sprintf(clientP2P->payload, "%lu", size);
    /* sending the size of the file to be sent */
    fillMsg((struct client *) clientP2P, clientP2P->userPseudo, FILE_SEND, "\0", clientP2P->payload);
    sendMsg(clientP2P->socket.fd, &clientP2P->msgStruct, clientP2P->payload);
    memset(clientP2P->payload, 0, MSG_LEN);

    long offset = 0;
    /* while file not totally sent */
    /* sending file by frames of 1024 Bytes */
    while (offset != size) {
        /* putting frame of 1024 Bytes in payload */
        long ret = read(fileFd, clientP2P->payload, MSG_LEN);
        if (-1 == ret) {
            perror("Reading from client socket");
            exit(EXIT_FAILURE);
        }
        if (0 == ret) {
            printf("Should close connection, read 0 bytes\n");
            close(fileFd);
            break;
        }
        clientP2P->msgStruct.payloadLen = ret;
        sendMsg(clientP2P->socket.fd, &clientP2P->msgStruct, clientP2P->payload);
        /* waiting data to be read by dest user in the socket file*/
        usleep(1000);
        offset += ret;
        /* displaying progression bar while sending file */
        char progressBar[12] = {'[', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
                                ']'}; //barre de progression de l'envoi du fichier
        int progression = (int) (((double) offset / (double) size) * 100);
        for (int j = 1; j <= progression / 10; j++) {
            progressBar[j] = '#';
        }
        for (int j = 0; j < (int) (sizeof(progressBar) / sizeof(char)); j++) {
            printf("%c", progressBar[j]);
        }
        printf(" [%i%%]\n", progression);
        printf("\033[1A\33[2K\r");
    }
    printf("File sent successfully\n");
    return 1;
}

void nicknameNew(struct client *client, char *newPseudo) {
    if (newPseudo == NULL) {
        printf("Please, choose a pseudo !\n");
        return;
    }
    unsigned long pseudoLength = strlen(newPseudo);
    /* checking length */
    unsigned long correctPseudoLength = strspn(newPseudo,"QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm1234567890");
    if (pseudoLength > NICK_LEN) {
        printf("Too long pseudo\n");
        return;
    }
    /* checking types of characters */
    if (correctPseudoLength != pseudoLength) {
        printf("Pseudo with non authorized characters, please, only numbers and letters\n");
        return;
    }

    fillMsg(client, client->userPseudo, NICKNAME_NEW, newPseudo, "\0");
    sendMsg(client->socket.fd, &client->msgStruct, client->payload);
    return;
}

void nicknameList(struct client *client) {
    fillMsg(client, client->userPseudo, NICKNAME_LIST, "\0", "\0");
    sendMsg(client->socket.fd, &client->msgStruct, client->payload);
    return;
}

void nicknameInfos(struct client *client, char *username) {
    fillMsg(client, client->userPseudo, NICKNAME_INFOS, username, "\0");
    sendMsg(client->socket.fd, &client->msgStruct, client->payload);
    return;
}

void broadcastSend(struct client *client, char *payload) {
    fillMsg(client, client->userPseudo, BROADCAST_SEND, "\0", payload);
    sendMsg(client->socket.fd, &client->msgStruct, client->payload);
    return;
}

void unicastSend(struct client *client, char *destUsername, char *payload) {
    /* checking dest user and payload are corrects */
    if (destUsername == NULL || payload == NULL) {
        printf("Invalid arguments\n");
        return;
    }

    fillMsg(client, client->userPseudo, UNICAST_SEND, destUsername, payload);
    sendMsg(client->socket.fd, &client->msgStruct, client->payload);
    return;
}

void multicastCreate(struct client *client, char *channelName) {
    if (channelName == NULL) {
        printf("No channel name\n");
        return;
    }
    unsigned long channelLength = strlen(channelName);
    /* checking length */
    unsigned long correct_channelLength = strspn(channelName,
                                                 "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm1234567890");
    /* checking types of characters */
    if (correct_channelLength != channelLength) {
        printf("Channel name with non authorized characters, please only numbers and letters\n");
        return;
    }

    fillMsg(client, client->userPseudo, MULTICAST_CREATE, channelName, "\0");
    sendMsg(client->socket.fd, &client->msgStruct, client->payload);
    return;
}

void multicastList(struct client *client) {
    fillMsg(client, client->userPseudo, MULTICAST_LIST, "\0", "\0");
    sendMsg(client->socket.fd, &client->msgStruct, client->payload);
    return;
}

void multicastJoin(struct client *client, char *chatroom) {
    fillMsg(client, client->userPseudo, MULTICAST_JOIN, chatroom, "\0");
    sendMsg(client->socket.fd, &client->msgStruct, client->payload);
    return;
}

int quit(struct client *client, char *channelName) {
    /* if channel name exists, quitting the channel */
    if (channelName == NULL) {
        /* quitting the server */
        return 0;
    }
    fillMsg(client, client->userPseudo, MULTICAST_QUIT, channelName, "\0");
    sendMsg(client->socket.fd, &client->msgStruct, client->payload);
    return 1;
}

void fileRequest(struct client *client, char *dstUser, char *filename) {
    /* getting path of file to send */
    if (dstUser == NULL || filename == NULL) {
        printf("Invalid arguments\n");
        return;
    }

    struct clientP2P *clientP2P = &client->clientP2P;
    strcpy(clientP2P->fileStruct.nickReceiver, dstUser);
    strcpy(clientP2P->fileStruct.fileToSend, filename + 1);
    clientP2P->fileStruct.fileToSend[strlen(clientP2P->fileStruct.fileToSend) - 1] = '\0';
    /* extracting name of file to send from the path */
    char tmp[MSG_LEN];
    strcpy(tmp, clientP2P->fileStruct.fileToSend);
    char subPath[MSG_LEN];
    extractFilename(tmp, subPath);
    /* writing only name of file to send and not the whole path into the payload */
    fillMsg(client, client->userPseudo, FILE_REQUEST, clientP2P->fileStruct.nickReceiver, subPath);
    sendMsg(client->socket.fd, &client->msgStruct, client->payload);
    /* Sending structure and payload */
    printf("File request sent to %s\n", client->clientP2P.fileStruct.nickReceiver);
    return;
}

void multicastSend(struct client *client, char *phrase, char *firstWord) {
    if (phrase != NULL) {
        sprintf(client->payload + strlen(client->payload), "%s %s", firstWord, phrase);
    } else {
        strcpy(client->payload, firstWord);
    }

    fillMsg(client, client->userPseudo, MULTICAST_SEND, "\0", client->payload);
    sendMsg(client->socket.fd, &client->msgStruct, client->payload);
    return;
}

/* processing data entered from keyboard */
int fromStdIn(struct client *client) {
    /* putting data into buffer */
    int n = 0;
    while ((client->buffer[n++] = (char) getchar()) != '\n') {}
    /* removing \n at the end of the buffer */
    client->buffer[strlen(client->buffer) - 1] = '\0';
    /* move to the beginning of previous line */
    printf("\033[1A\33[2K\r");
    fflush(stdout);
    /* getting command entered by user */
    char *firstWord = strtok(client->buffer, " ");
    if (firstWord == NULL) {
        return 1;
    }
    /* getting type of message */
    enum msgType msgType = getMsgType(firstWord);
    /* asking user to log in first */
    if (msgType != NICKNAME_NEW && client->loggedIn == 0) {
        printf("Please, login with /nick <your pseudo>\n");
        return 1;
    }

    /* /nick <pseudo> */
    if (msgType == NICKNAME_NEW) {
        nicknameNew(client, strtok(NULL, " "));
        return 1;
    } /* /help */
    else if (msgType == HELP) {
        help();
        return 1;
    } /* /who */
    else if (msgType == NICKNAME_LIST) {
        nicknameList(client);
        return 1;
    } /* /whois <user> */
    else if (msgType == NICKNAME_INFOS) {
        nicknameInfos(client, strtok(NULL, ""));
        return 1;
    } /* /msgall msg */
    else if (msgType == BROADCAST_SEND) {
        broadcastSend(client, strtok(NULL, ""));
        return 1;
    } /* /msg <user> msg */
    else if (msgType == UNICAST_SEND) {
        char *destUsername = strtok(NULL, " ");
        char *payload = strtok(NULL, "");
        unicastSend(client, destUsername, payload);
        return 1;
    } /* /create <channel name> */
    else if (msgType == MULTICAST_CREATE) {
        multicastCreate(client, strtok(NULL, ""));
        return 1;
    } /* /channel_list */
    else if (msgType == MULTICAST_LIST) {
        multicastList(client);
        return 1;
    } /* /join <channel name> */
    else if (msgType == MULTICAST_JOIN) {
        multicastJoin(client, strtok(NULL, ""));
        return 1;
    } /* /quit <channel name> */
    else if (msgType == QUIT) {
        if (!quit(client, strtok(NULL, ""))) {
            return 0;
        }
        return 1;
    } /* /send <user> "file" */
    else if (msgType == FILE_REQUEST) {
        char *destUser = strtok(NULL, " ");
        char *filenamme = strtok(NULL, "");
        fileRequest(client, destUser, filenamme);
        return 1;
    } else {
        multicastSend(client, strtok(NULL, ""), firstWord);
        return 1;
    }
}

/* connecting to the server and sending file */
void connectAndSendFile(struct clientP2P *clientP2P) {
    /* getting ip address and port to connect to the server */
    char *ipAddr = strtok(clientP2P->buffer, ":");
    char *portNumber = strtok(NULL, "\n");
    /* connecting to the server */
    clientP2P->socket = socketAndConnect(ipAddr, portNumber);
    printf("You (%s:%hu) are now connected to %s (%s:%s)\n", clientP2P->socket.ipAddr,
           clientP2P->socket.port, clientP2P->fileStruct.nickReceiver, ipAddr, portNumber);
    /* sending file */
    printf("Sending the file...\n");
    if (!sendFile(clientP2P)) {
        return;
    }
    /* receiving ack */
    recv(clientP2P->socket.fd, &clientP2P->msgStruct, sizeof(struct message), MSG_WAITALL);
    if (clientP2P->msgStruct.type == FILE_ACK) {
        printf("%s has received the file\n", clientP2P->fileStruct.nickReceiver);
    } else {
        printf("Problem in reception\n");
    }
    /* closing connection */
    close(clientP2P->socket.fd);
    printf("Connection closed with %s\n", clientP2P->fileStruct.nickReceiver);
}

void fileAccept(struct client *client) {
    /* letting the computer choosing a listening port */
    char listeningPort[INFOS_LEN] = "0";
    /* creating a listening socket */
    struct clientP2P *serverP2P = &client->serverP2P;
    serverP2P->socket = bindAndListen(listeningPort);
    sprintf(serverP2P->payload, "%s:%hu", serverP2P->socket.ipAddr, serverP2P->socket.port);
    /* sending ip address and port for the client to connect */
    fillMsg((struct client *) serverP2P, serverP2P->userPseudo, FILE_ACCEPT, serverP2P->fileStruct.nickSender,
            serverP2P->payload);
    sendMsg(client->socket.fd, &serverP2P->msgStruct, serverP2P->payload);
    struct sockaddr client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t len = sizeof(client_addr);
    /* accepting connection from client */
    if ((serverP2P->socket.fd = accept(serverP2P->socket.fd, &client_addr, &len)) == -1) {
        perror("Accept");
    }
    getpeername(serverP2P->socket.fd, (struct sockaddr *) &client_addr, &len);
    strcpy(serverP2P->socket.ipAddr, inet_ntoa(((struct sockaddr_in *) &client_addr)->sin_addr));
    serverP2P->socket.port = ntohs(((struct sockaddr_in *) &client_addr)->sin_port);
    printf("%s (%s:%i) is now connected to you\n", serverP2P->fileStruct.nickSender, serverP2P->socket.ipAddr,
           serverP2P->socket.port);
    /* receiving file */
    printf("You accepted the file transfer\n");

    printf("Receiving the file...\n");
    /* receiving a first structure with the name of the file to receive */
    if (recv(serverP2P->socket.fd, &serverP2P->msgStruct, sizeof(struct message), MSG_WAITALL) <= 0) {
        perror("recv FILE_SEND");
        exit(EXIT_FAILURE);
    }
    /* if name of the file to send did not exist on the client computer
     * file transfer is aborted */
    if (serverP2P->msgStruct.type == FILENAME) {
        printf("Error: file not sent\n");
        recv(serverP2P->socket.fd, serverP2P->buffer, 0, MSG_WAITALL);
        close(serverP2P->socket.fd);
        printf("Connection closed with %s\n", serverP2P->fileStruct.nickSender);
        return;
    }
    /* receiving the size of the file */
    if (recv(serverP2P->socket.fd, serverP2P->buffer, serverP2P->msgStruct.payloadLen, MSG_WAITALL) <= 0) {
        perror("recv file size");
        exit(EXIT_FAILURE);
    }
    downloadingFile(serverP2P);
    serverP2P->msgStruct.type = FILE_ACK;
    /* sending ack */
    send(serverP2P->socket.fd, &serverP2P->msgStruct, sizeof(struct message), 0);
    recv(serverP2P->socket.fd, serverP2P->buffer, 0, MSG_WAITALL);
    close(serverP2P->socket.fd);
    printf("Connection closed with %s\n", serverP2P->fileStruct.nickSender);
    return;
}

void fileReject(struct client *client) {
    fillMsg(client, client->userPseudo, FILE_REJECT, client->msgStruct.infos, client->buffer);
    /* Sending structure and payload */
    sendMsg(client->socket.fd, &client->msgStruct, client->payload);
    printf("You rejected the file transfer\n");
}

/* processing data coming from the server */
int fromServer(struct client *client) {
    /*Receiving structure*/
    if (recv(client->socket.fd, &client->msgStruct, sizeof(struct message), MSG_WAITALL) <= 0) {
        printf("Server has crashed\n");
        return 0;
    }
    /* Receiving message*/
    if (client->msgStruct.payloadLen != 0 && recv(client->socket.fd, client->buffer, client->msgStruct.payloadLen, MSG_WAITALL) <= 0) {
        perror("recv");
        return 0;
    }
    /* changing pseudo */
    if (client->msgStruct.type == NICKNAME_NEW) {
        strcpy(client->userPseudo, client->msgStruct.infos);
        /* user logged in */
        client->loggedIn = 1;
    } /* if receiving a file request */
    if (client->msgStruct.type == FILE_REQUEST) {
        printf("[%s]: %s wants you to accept the transfer of the file named \"%s\". Do you accept ? [Y/N]\n",
               client->msgStruct.nickSender, client->msgStruct.infos, client->buffer);
        strcpy(client->serverP2P.fileStruct.fileToReceive, client->buffer);
        strcpy(client->serverP2P.fileStruct.nickSender, client->msgStruct.infos);
        while (1) {
            memset(client->buffer, 0, MSG_LEN);
            int n = 0;
            while ((client->buffer[n++] = (char) getchar()) != '\n') {}
            /* removing \n at the end of the buffer */
            client->buffer[strlen(client->buffer) - 1] = '\0';
            /* move to the beginning of previous line */
            printf("\033[1A\33[2K\r");
            fflush(stdout);
            enum msgType P2PresponseType = getMsgType(client->buffer);
            if (P2PresponseType == FILE_ACCEPT) {
                fileAccept(client);
                return 1;
            } else if (P2PresponseType == FILE_REJECT) {
                fileReject(client);
                return 1;
            } else {
                printf("Please only Y or N !\n");
            }
        }
    } /* if receiving a file accept */
    if (client->msgStruct.type == FILE_ACCEPT) {
        printf("[SERVER]: %s accepted file transfer\n", client->msgStruct.nickSender);
        strcpy(client->clientP2P.buffer, client->buffer);
        connectAndSendFile(&client->clientP2P);
        return 1;
    }
    printf("[%s]: %s\n", client->msgStruct.nickSender, client->buffer);
    return 1;
}

int runClient(struct client *client) {
    client->clientP2P.userPseudo = client->userPseudo;
    client->serverP2P.userPseudo = client->userPseudo;
    /* Declare array of struct pollfd */
    struct pollfd pollfds[NFDS];
    /* Init first slots with listening socket  to receive data */
    /* from server */
    pollfds[0].fd = client->socket.fd;
    pollfds[0].events = POLLIN;
    pollfds[0].revents = 0;
    /* from keyboard */
    pollfds[1].fd = STDIN_FILENO;
    pollfds[1].events = POLLIN;
    pollfds[1].revents = 0;
    printf("Please, login with /nick <your pseudo>\n");
    /* client loop */
    while (1) {
        fflush(stdout);
        if (-1 == poll(pollfds, NFDS, -1)) {
            perror("Poll");
        }
        for (int i = 0; i < NFDS; i++) {
            memset(&client->msgStruct, 0, sizeof(struct message));
            memset(client->buffer, 0, MSG_LEN);
            memset(client->payload, 0, MSG_LEN);
            /* if data comes from keyboard */
            if (pollfds[i].fd == STDIN_FILENO && pollfds[i].revents & POLLIN) {
                strcpy(client->msgStruct.nickSender, client->userPseudo);
                if (!fromStdIn(client)) {
                    disconnectFromServer(pollfds);
                    return 1;
                }
                pollfds[i].revents = 0;
            } /* if data comes from server */
            else if (pollfds[i].fd != STDIN_FILENO && pollfds[i].revents & POLLIN) {
                if (!fromServer(client)) {
                    disconnectFromServer(pollfds);
                    return 0;
                }
                pollfds[i].revents = 0;
            }
        }
    }
}

struct client *clientInit(char* hostname, char *port) {
    struct client *client = malloc(sizeof(struct client));
    memset(client, 0, sizeof(struct client));
    client->socket = socketAndConnect(hostname, port);
    printf("Client (%s:%hu) connected to server (%s:%s)\n", client->socket.ipAddr, client->socket.port, hostname, port);
    return client;
}

void usage(int argc) {
    if (argc != 3) {
        printf("Usage: ./client hostname portNumber\n");
        exit(EXIT_FAILURE);
    }
}
int main(int argc, char *argv[]) {
    usage(argc);

    struct client *client = clientInit(argv[1], argv[2]);
    /* running client */
    if (!runClient(client)) {
        free(client);
        exit(EXIT_FAILURE);
    }

    free(client);
    exit(EXIT_SUCCESS);
}
