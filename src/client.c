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

void nicknameNew(struct client *client, char *newPseudo) {
    if (newPseudo == NULL) {
        printf("Please, choose a pseudo !\n");
        return;
    }
    unsigned long pseudoLength = strlen(newPseudo);
    /* checking length */
    unsigned long correctPseudoLength = strspn(newPseudo,
                                               "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm1234567890");
    if (pseudoLength > NICK_LEN) {
        printf("Too long pseudo\n");
        return;
    }
    /* checking types of characters */
    if (correctPseudoLength != pseudoLength) {
        printf("Pseudo with non authorized characters, please, only numbers and letters\n");
        return;
    }

    fillPacket(&client->packet, client->userPseudo, NICKNAME_NEW, newPseudo, "\0");
    sendPacket(client->socket.fd, &client->packet);
    return;
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

void nicknameList(struct client *client) {
    fillPacket(&client->packet, client->userPseudo, NICKNAME_LIST, "\0", "\0");
    sendPacket(client->socket.fd, &client->packet);
    return;
}

void nicknameInfos(struct client *client, char *username) {
    fillPacket(&client->packet, client->userPseudo, NICKNAME_INFOS, username, "\0");
    sendPacket(client->socket.fd, &client->packet);
    return;
}

void broadcastSend(struct client *client, char *payload) {
    fillPacket(&client->packet, client->userPseudo, BROADCAST_SEND, "\0", payload);
    sendPacket(client->socket.fd, &client->packet);
    return;
}

void unicastSend(struct client *client, char *destUsername, char *payload) {
    /* checking dest user and payload are corrects */
    if (destUsername == NULL || payload == NULL) {
        printf("Invalid arguments\n");
        return;
    }

    fillPacket(&client->packet, client->userPseudo, UNICAST_SEND, destUsername, payload);
    sendPacket(client->socket.fd, &client->packet);
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

    fillPacket(&client->packet, client->userPseudo, MULTICAST_CREATE, channelName, "\0");
    sendPacket(client->socket.fd, &client->packet);
    return;
}

void multicastList(struct client *client) {
    fillPacket(&client->packet, client->userPseudo, MULTICAST_LIST, "\0", "\0");
    sendPacket(client->socket.fd, &client->packet);
    return;
}

void multicastJoin(struct client *client, char *chatroom) {
    fillPacket(&client->packet, client->userPseudo, MULTICAST_JOIN, chatroom, "\0");
    sendPacket(client->socket.fd, &client->packet);
    return;
}

int quit(struct client *client, char *channelName) {
    /* if channel name exists, quitting the channel */
    if (channelName == NULL) {
        /* quitting the server */
        return 0;
    }
    fillPacket(&client->packet, client->userPseudo, MULTICAST_QUIT, channelName, "\0");
    sendPacket(client->socket.fd, &client->packet);
    return 1;
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

void fileRequest(struct client *client, char *dstUser, char *filename) {
    /* getting path of file to send */
    if (dstUser == NULL || filename == NULL) {
        printf("Invalid arguments\n");
        return;
    }

    strcpy(client->fileToSend, filename + 1);
    client->fileToSend[strlen(client->fileToSend) - 1] = '\0';
    /* extracting name of file to send from the path */
    char tmp[MSG_LEN];
    strcpy(tmp, client->fileToSend);
    char file[MSG_LEN];
    extractFilename(tmp, file);
    /* writing only name of file to send and not the whole path into the payload */
    fillPacket(&client->packet, client->userPseudo, FILE_REQUEST, dstUser, file);
    sendPacket(client->socket.fd, &client->packet);
    /* Sending structure and payload */
    printf("File request sent to %s\n", dstUser);
    return;
}

void multicastSend(struct client *client, char *phrase, char *firstWord) {
    if (phrase != NULL) {
        sprintf(client->packet.payload, "%s %s", firstWord, phrase);
    } else {
        strcpy(client->packet.payload, firstWord);
    }

    fillPacket(&client->packet, client->userPseudo, MULTICAST_SEND, "\0", client->packet.payload);
    sendPacket(client->socket.fd, &client->packet);
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

    /* asking user to log in first */
    if (strcmp(firstWord, "/nick") != 0 && client->loggedIn == 0) {
        printf("Please, login with /nick <your pseudo>\n");
        return 1;
    }

    if (strcmp(firstWord, "/nick") == 0) {
        nicknameNew(client, strtok(NULL, " "));
        return 1;
    } else if (strcmp(firstWord, "/help") == 0) {
        help();
        return 1;
    } else if (strcmp(firstWord, "/who") == 0) {
        nicknameList(client);
        return 1;
    } else if (strcmp(firstWord, "/whois") == 0) {
        nicknameInfos(client, strtok(NULL, ""));
        return 1;
    } else if (strcmp(firstWord, "/msgall") == 0) {
        broadcastSend(client, strtok(NULL, ""));
        return 1;
    } else if (strcmp(firstWord, "/msg") == 0) {
        char *destUsername = strtok(NULL, " ");
        char *payload = strtok(NULL, "");
        unicastSend(client, destUsername, payload);
        return 1;
    } else if (strcmp(firstWord, "/create") == 0) {
        multicastCreate(client, strtok(NULL, ""));
        return 1;
    } else if (strcmp(firstWord, "/channel_list") == 0) {
        multicastList(client);
        return 1;
    } else if (strcmp(firstWord, "/join") == 0) {
        multicastJoin(client, strtok(NULL, ""));
        return 1;
    } else if (strcmp(firstWord, "/quit") == 0) {
        if (!quit(client, strtok(NULL, ""))) {
            return 0;
        }
        return 1;
    } else if (strcmp(firstWord, "/send") == 0) {
        char *destUser = strtok(NULL, " ");
        char *filename = strtok(NULL, "");
        fileRequest(client, destUser, filename);
        return 1;
    } else {
        multicastSend(client, strtok(NULL, ""), firstWord);
        return 1;
    }
}

void nicknameNewFromServer(struct client *client) {
    strcpy(client->userPseudo, client->packet.header.infos);
    /* user logged in */
    client->loggedIn = 1;
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

void fileAcceptFromStdIn(struct client *client, char *fileSender, char *fileToReceive) {
    printf("You accepted the file transfer\n");
    /* letting the computer choosing a listening port */
    char listeningPort[INFOS_LEN] = "0";
    /* creating a listening socket */
    struct client *serverP2P = malloc(sizeof(struct client));
    memset(serverP2P, 0, sizeof(struct client));
    serverP2P->socket = bindAndListen(listeningPort);
    sprintf(serverP2P->packet.payload, "%s:%hu", serverP2P->socket.ipAddr, serverP2P->socket.port);
    /* sending ip address and port for the client to connect */
    fillPacket(&serverP2P->packet, client->userPseudo, FILE_ACCEPT, fileSender, serverP2P->packet.payload);
    sendPacket(client->socket.fd, &serverP2P->packet);
    struct sockaddr client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t len = sizeof(client_addr);
    /* accepting connection from client */
    if ((serverP2P->socket.fd = accept(serverP2P->socket.fd, &client_addr, &len)) == -1) {
        perror("Accept");
    }
    getpeername(serverP2P->socket.fd, (struct sockaddr *) &client_addr, &len);
    char clientIpAddr[INFOS_LEN];
    strcpy(clientIpAddr, inet_ntoa(((struct sockaddr_in *) &client_addr)->sin_addr));
    u_short clientPort = ntohs(((struct sockaddr_in *) &client_addr)->sin_port);
    printf("%s (%s:%i) is now connected to you\n", fileSender, clientIpAddr, clientPort);

    /* receiving file */
    printf("Receiving the file...\n");

    /* receiving a first structure to check if file exists on client's computer and if yes, with the size of the file */
    if (recv(serverP2P->socket.fd, &serverP2P->packet.header, sizeof(struct header), MSG_WAITALL) <= 0) {
        perror("recv FILE_SEND");
        exit(EXIT_FAILURE);
    }

    /* if name of the file to send did not exist on the client's computer
     * file transfer is aborted */
    if (serverP2P->packet.header.type == FILENAME) {
        printf("Error: file not sent\n");
        recv(serverP2P->socket.fd, serverP2P->buffer, 0, MSG_WAITALL);
        close(serverP2P->socket.fd);
        printf("Connection closed with %s (%s:%hu)\n", fileSender, clientIpAddr, clientPort);
        free(serverP2P);
        return;
    }

    /* receiving the size of the file */
    if (recv(serverP2P->socket.fd, serverP2P->buffer, serverP2P->packet.header.payloadLen, MSG_WAITALL) <= 0) {
        perror("recv file size");
        exit(EXIT_FAILURE);
    }
    /* receiving data of the file from client and saving it into a new file on our computer */
    /* creating directory where the received file will be stored */
    if (mkdir("inbox", S_IRWXU | S_IRWXG | S_IRWXO) && errno != EEXIST) {
        perror("mkdir:");
    }
    char filenameDst[NICK_LEN];
    /* creating the path of the received file */
    sprintf(filenameDst, "inbox/%s", fileToReceive);
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
        recv(serverP2P->socket.fd, &serverP2P->packet.header, sizeof(struct header), MSG_WAITALL);
        /* receiving file by frame of max 1024 bytes */
        recv(serverP2P->socket.fd, serverP2P->buffer, serverP2P->packet.header.payloadLen, MSG_WAITALL);
        /* writing received data in a new file */
        if (-1 == (ret = write(newFileFd, serverP2P->buffer, serverP2P->packet.header.payloadLen))) {
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

    serverP2P->packet.header.type = FILE_ACK;
    /* sending ack */
    send(serverP2P->socket.fd, &serverP2P->packet.header, sizeof(struct header), 0);
    recv(serverP2P->socket.fd, serverP2P->buffer, 0, MSG_WAITALL);
    close(serverP2P->socket.fd);
    printf("Connection closed with %s\n", fileSender);
    free(serverP2P);
    return;
}

void fileReject(struct client *client) {
    memset(client->fileToSend, 0, NICK_LEN);
    fillPacket(&client->packet, client->userPseudo, FILE_REJECT, client->packet.header.infos, client->buffer);
    /* Sending structure and payload */
    sendPacket(client->socket.fd, &client->packet);
    printf("You rejected the file transfer\n");
}

void fileRequestFromServer(struct client *client) {
    printf("[%s]: %s wants you to accept the transfer of the file named \"%s\". Do you accept ? [Y/N]\n",
           client->packet.header.nickSender, client->packet.header.infos, client->buffer);

    /* loop waiting for user response for file request */
    while (1) {
        memset(client->buffer, 0, MSG_LEN);
        int n = 0;
        while ((client->buffer[n++] = (char) getchar()) != '\n') {}
        /* removing \n at the end of the buffer */
        client->buffer[strlen(client->buffer) - 1] = '\0';
        /* move to the beginning of previous line */
        printf("\033[1A\33[2K\r");
        fflush(stdout);

        if (strcmp(client->buffer, "Y") == 0 || strcmp(client->buffer, "y") == 0) {
            fileAcceptFromStdIn(client, client->packet.header.infos, client->buffer);
            return;
        } else if (strcmp(client->buffer, "N") == 0 || strcmp(client->buffer, "n") == 0) {
            fileReject(client);
            return;
        } else {
            printf("Please only Y or N !\n");
            break;
        }
    }
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

/* connecting to the server and sending file */
void fileAcceptFromServer(struct client *client) {
    printf("[SERVER]: %s accepted file transfer\n", client->packet.header.nickSender);
    /* getting ip address and port to connect to the server */
    char *serverIpAddr = strtok(client->buffer, ":");
    char *serverPortNumber = strtok(NULL, "\n");
    /* connecting to the server */
    struct client *clientP2P = malloc(sizeof(struct client));
    memset(clientP2P, 0, sizeof(struct client));
    clientP2P->socket = socketAndConnect(serverIpAddr, serverPortNumber);
    printf("You (%s:%hu) are now connected to %s (%s:%s)\n", clientP2P->socket.ipAddr,
           clientP2P->socket.port, client->packet.header.nickSender, serverIpAddr, serverPortNumber);
    /* sending file */
    printf("Sending the file...\n");

    struct stat sb;
    memset(&sb, 0, sizeof(struct stat));
    stat(client->fileToSend, &sb);
    if (!S_ISREG(sb.st_mode)) {
        printf("Invalid filename\n");
        clientP2P->packet.header.type = FILENAME;
        send(clientP2P->socket.fd, &clientP2P->packet.header, sizeof(struct header), 0);
        close(clientP2P->socket.fd);
        free(clientP2P);
        printf("Connection closed with %s\n", client->packet.header.nickSender);
        return;
    }
    /* opening file to send */
    int fileFd = open(client->fileToSend, O_RDONLY);
    if (fileFd == -1) {
        if (errno == ENOENT) {
            printf("Invalid filename\n");
            clientP2P->packet.header.type = FILENAME;
            send(clientP2P->socket.fd, &clientP2P->packet.header, sizeof(struct header), 0);
            close(clientP2P->socket.fd);
            printf("Connection closed with %s\n", client->packet.header.nickSender);
            free(clientP2P);
            return;
        }
        perror("Error opening file");
        exit(EXIT_FAILURE);

    }
    /* getting the size of the file to send */
    long size = sb.st_size;
    sprintf(clientP2P->packet.payload, "%lu", size);
    /* sending the size of the file to be sent */
    fillPacket(&clientP2P->packet, client->userPseudo, FILE_SEND, "\0", clientP2P->packet.payload);
    sendPacket(clientP2P->socket.fd, &clientP2P->packet);
    memset(clientP2P->packet.payload, 0, MSG_LEN);

    long offset = 0;
    /* while file not totally sent */
    /* sending file by frames of 1024 Bytes */
    while (offset != size) {
        /* putting frame of 1024 Bytes in payload */
        long ret = read(fileFd, clientP2P->packet.payload, MSG_LEN);
        if (-1 == ret) {
            perror("Reading from client socket");
            exit(EXIT_FAILURE);
        }
        if (0 == ret) {
            printf("Should close connection, read 0 bytes\n");
            close(fileFd);
            break;
        }
        clientP2P->packet.header.payloadLen = ret;
        sendPacket(clientP2P->socket.fd, &clientP2P->packet);
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

    /* receiving ack */
    recv(clientP2P->socket.fd, &clientP2P->packet.header, sizeof(struct header), MSG_WAITALL);
    if (clientP2P->packet.header.type == FILE_ACK) {
        printf("%s has received the file\n", client->packet.header.nickSender);
    } else {
        printf("Problem in reception\n");
    }
    /* closing connection */
    close(clientP2P->socket.fd);
    free(clientP2P);
    printf("Connection closed with %s\n", client->packet.header.nickSender);
}

/* processing data coming from the server */
int fromServer(struct client *client) {
    /*Receiving structure*/
    if (recv(client->socket.fd, &client->packet.header, sizeof(struct header), MSG_WAITALL) <= 0) {
        printf("Server has crashed\n");
        return 0;
    }
    /* Receiving message*/
    if (client->packet.header.payloadLen != 0 &&
        recv(client->socket.fd, client->buffer, client->packet.header.payloadLen, MSG_WAITALL) <= 0) {
        perror("recv");
        return 0;
    }

    switch (client->packet.header.type) {
        /* changing pseudo */
        case NICKNAME_NEW:
            nicknameNewFromServer(client);
            break;
            /* if receiving a file request */
        case FILE_REQUEST:
            fileRequestFromServer(client);
            return 1;
            /* if receiving a file accept */
        case FILE_ACCEPT:
            fileAcceptFromServer(client);
            return 1;
        default:
            break;
    }

    printf("[%s]: %s\n", client->packet.header.nickSender, client->buffer);
    return 1;
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

int runClient(struct client *client) {
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
            memset(&client->packet.header, 0, sizeof(struct header));
            memset(client->buffer, 0, MSG_LEN);
            memset(client->packet.payload, 0, MSG_LEN);
            /* if data comes from keyboard */
            if (pollfds[i].fd == STDIN_FILENO && pollfds[i].revents & POLLIN) {
                strcpy(client->packet.header.nickSender, client->userPseudo);
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

struct client *clientInit(char *hostname, char *port) {
    struct client *client = malloc(sizeof(struct client));
    memset(client, 0, sizeof(struct client));
    client->socket = socketAndConnect(hostname, port);
    printf("Client (%s:%hu) connected to server (%s:%s)\n", client->socket.ipAddr, client->socket.port, hostname, port);
    return client;
}

void usage() {
    printf("Usage: ./client hostname portNumber\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        usage();
    }

    struct client *client = clientInit(argv[1], argv[2]);
    /* running client */
    if (!runClient(client)) {
        free(client);
        exit(EXIT_FAILURE);
    }

    free(client);
    exit(EXIT_SUCCESS);
}
