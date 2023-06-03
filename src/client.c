#include "../include/client.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void nickname_new(struct Client *client, char *new_nickname) {
    if (new_nickname == NULL) {
        printf("Please, choose a nickname !\n");
        return;
    }
    unsigned long len_nickname = strlen(new_nickname);
    /* checking length */
    unsigned long len_correct_nickname = strspn(new_nickname, "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm1234567890");
    if (len_nickname > NICK_LEN) {
        printf("Too long nickname\n");
        return;
    }
    /* checking types of characters */
    if (len_correct_nickname != len_nickname) {
        printf("Nickname with non authorized characters, please, only numbers and letters\n");
        return;
    }

    packet_set(&client->packet, client->nickname, NICKNAME_NEW, new_nickname, "\0");
    packet_send(&client->packet,  client->fd_socket);
    return;
}

void help() {
    printf("            ****** HELP ******\n");
    printf("- /nick <nickname> to change nickname\n");
    printf("- /whois <nickname> to have informations on user <nickname>\n");
    printf("- /who to have the list of online users\n");
    printf("- /msgall to send a message to all user\n");
    printf("- /msg <nickname> to send a message to user <nickname>\n");
    printf("- /create <name_channel> to create a chat group <name_channel>\n");
    printf("- /channel_list to have the list of all online chat groups\n");
    printf("- /join <name_channel> to join a chat group <name_channel>\n");
    printf("- /quit <name_channel> to quit chat group <name_channel>\n");
    printf("- /quit to disconnect\n");
    printf("- /send <nickname> <\"filename\"> to send a file <filename> to user <nickname>\n");
}

void nickname_list(struct Client *client) {
    packet_set(&client->packet, client->nickname, NICKNAME_LIST, "\0", "\0");
    packet_send(&client->packet,  client->fd_socket);
    return;
}

void nickname_infos(struct Client *client, char *nickname) {
    packet_set(&client->packet, client->nickname, NICKNAME_INFOS, nickname, "\0");
    packet_send(&client->packet,  client->fd_socket);
    return;
}

void broadcast_send(struct Client *client, char *payload) {
    packet_set(&client->packet, client->nickname, BROADCAST_SEND, "\0", payload);
    packet_send(&client->packet,  client->fd_socket);
    return;
}

void unicast_send(struct Client *client, char *nickname_dest, char *payload) {
    /* checking dest user and payload are corrects */
    if (nickname_dest == NULL || payload == NULL) {
        printf("Invalid arguments\n");
        return;
    }

    packet_set(&client->packet, client->nickname, UNICAST_SEND, nickname_dest, payload);
    packet_send(&client->packet,  client->fd_socket);
    return;
}

void multicast_create(struct Client *client, char *name_channel) {
    if (name_channel == NULL) {
        printf("No channel name\n");
        return;
    }
    unsigned long len_channel = strlen(name_channel);
    /* checking length */
    unsigned long correct_len_channel = strspn(name_channel, "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm1234567890");
    /* checking types of characters */
    if (correct_len_channel != len_channel) {
        printf("Channel name with non authorized characters, please only numbers and letters\n");
        return;
    }

    packet_set(&client->packet, client->nickname, MULTICAST_CREATE, name_channel, "\0");
    packet_send(&client->packet,  client->fd_socket);
    return;
}

void multicast_list(struct Client *client) {
    packet_set(&client->packet, client->nickname, MULTICAST_LIST, "\0", "\0");
    packet_send(&client->packet,  client->fd_socket);
    return;
}

void multicast_join(struct Client *client, char *chatroom) {
    packet_set(&client->packet, client->nickname, MULTICAST_JOIN, chatroom, "\0");
    packet_send(&client->packet,  client->fd_socket);
    return;
}

int quit(struct Client *client, char *name_channel) {
    /* if channel name exists, quitting the channel */
    if (name_channel == NULL) {
        /* quitting the server */
        return 0;
    }
    packet_set(&client->packet, client->nickname, MULTICAST_QUIT, name_channel, "\0");
    packet_send(&client->packet,  client->fd_socket);
    return 1;
}

/* extracting name of file to send */
void extract_filename(char *tmp, char *sub_path) {
    tmp = strtok(tmp, "/");
    strcpy(sub_path, tmp);
    /* extracting the name of the file from the whole path */
    while ((tmp = strtok(NULL, "/")) != NULL) {
        memset(sub_path, 0, NICK_LEN);
        strcpy(sub_path, tmp);
    }
}

void file_request(struct Client *client, char *nickname_dest, char *filename) {
    /* getting path of file to send */
    if (nickname_dest == NULL || filename == NULL) {
        printf("Invalid arguments\n");
        return;
    }

    strcpy(client->file_to_send, filename + 1);
    client->file_to_send[strlen(client->file_to_send) - 1] = '\0';
    /* extracting name of file to send from the path */
    char tmp[MSG_LEN];
    strcpy(tmp, client->file_to_send);
    char file[MSG_LEN];
    extract_filename(tmp, file);
    /* writing only name of file to send and not the whole path into the payload */
    packet_set(&client->packet, client->nickname, FILE_REQUEST, nickname_dest, file);
    packet_send(&client->packet,  client->fd_socket);
    /* Sending structure and payload */
    printf("File request sent to %s\n", nickname_dest);
    return;
}

void multicast_send(struct Client *client, char *phrase, char *first_word) {
    if (phrase != NULL) {
        sprintf(client->packet.payload, "%s %s", first_word, phrase);
    } else {
        strcpy(client->packet.payload, first_word);
    }

    packet_set(&client->packet, client->nickname, MULTICAST_SEND, "\0", client->packet.payload);
    packet_send(&client->packet,  client->fd_socket);
    return;
}

/* processing data entered from keyboard */
int from_stdin(struct Client *client) {
    /* putting data into buffer */
    int n = 0;
    while ((client->buffer[n++] = (char) getchar()) != '\n') {}
    /* removing \n at the end of the buffer */
    client->buffer[strlen(client->buffer) - 1] = '\0';
    /* move to the beginning of previous line */
    printf("\033[1A\33[2K\r");
    fflush(stdout);

    /* getting command entered by user */
    char *first_word = strtok(client->buffer, " ");
    if (first_word == NULL) {
        return 1;
    }

    if (strcmp(first_word, "/nick") == 0) {
        nickname_new(client, strtok(NULL, " "));
        return 1;
    } else if (strcmp(first_word, "/help") == 0) {
        help();
        return 1;
    } else if (strcmp(first_word, "/who") == 0) {
        nickname_list(client);
        return 1;
    } else if (strcmp(first_word, "/whois") == 0) {
        nickname_infos(client, strtok(NULL, ""));
        return 1;
    } else if (strcmp(first_word, "/msgall") == 0) {
        broadcast_send(client, strtok(NULL, ""));
        return 1;
    } else if (strcmp(first_word, "/msg") == 0) {
        char *nickname_dest = strtok(NULL, " ");
        char *payload = strtok(NULL, "");
        unicast_send(client, nickname_dest, payload);
        return 1;
    } else if (strcmp(first_word, "/create") == 0) {
        multicast_create(client, strtok(NULL, ""));
        return 1;
    } else if (strcmp(first_word, "/channel_list") == 0) {
        multicast_list(client);
        return 1;
    } else if (strcmp(first_word, "/join") == 0) {
        multicast_join(client, strtok(NULL, ""));
        return 1;
    } else if (strcmp(first_word, "/quit") == 0) {
        if (!quit(client, strtok(NULL, ""))) {
            return 0;
        }
        return 1;
    } else if (strcmp(first_word, "/send") == 0) {
        char *nickname_dest = strtok(NULL, " ");
        char *filename = strtok(NULL, "");
        file_request(client, nickname_dest, filename);
        return 1;
    } else {
        multicast_send(client, strtok(NULL, ""), first_word);
        return 1;
    }
}

void nickname_new_from_server(struct Client *client) {
    strcpy(client->nickname, client->packet.header.infos);
}

/* creation of a server socket */
struct Client *server_p2p_init(char *listening_port) {
    struct Client *server_p2p = malloc(sizeof(struct Client));
    memset(server_p2p, 0, sizeof(struct Client));
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, listening_port, &hints, &result) != 0) {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        server_p2p->fd_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (server_p2p->fd_socket == -1) {
            continue;
        }
        if (bind(server_p2p->fd_socket, rp->ai_addr, rp->ai_addrlen) == 0) {
            /* getting ip address  and port*/
            struct sockaddr_in my_addr;
            socklen_t len = sizeof(my_addr);
            getsockname(server_p2p->fd_socket, (struct sockaddr *) &my_addr, &len);
            inet_ntop(AF_INET, &my_addr.sin_addr, server_p2p->ip_addr, 16);
            server_p2p->port_num = ntohs(my_addr.sin_port);
            freeaddrinfo(result);
            if ((listen(server_p2p->fd_socket, SOMAXCONN)) != 0) {
                perror("listen()\n");
                exit(EXIT_FAILURE);
            }
            printf("Listening on %s:%hu\n", server_p2p->ip_addr, server_p2p->port_num);
            return server_p2p;
        }
        close(server_p2p->fd_socket);
    }
    perror("bind()");
    exit(EXIT_FAILURE);
}

void file_accept_from_stdin(struct Client *client, char *file_sender) {
    printf("You accepted the file transfer\n");
    /* letting the computer choosing a listening port */
    char listening_port[INFOS_LEN] = "0";
    /* creating a listening socket */
    struct Client *server_p2p = server_p2p_init(listening_port);
    sprintf(server_p2p->packet.payload, "%s:%hu", server_p2p->ip_addr, server_p2p->port_num);
    /* sending ip address and port for the client to connect */
    packet_set(&server_p2p->packet, client->nickname, FILE_ACCEPT, file_sender, server_p2p->packet.payload);
    packet_send(&server_p2p->packet, client->fd_socket);
    struct sockaddr client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t len = sizeof(client_addr);
    /* accepting connection from client */
    if ((server_p2p->fd_socket = accept(server_p2p->fd_socket, &client_addr, &len)) == -1) {
        perror("Accept");
    }
    getpeername(server_p2p->fd_socket, (struct sockaddr *) &client_addr, &len);
    char ip_addr_client[INFOS_LEN];
    strcpy(ip_addr_client, inet_ntoa(((struct sockaddr_in *) &client_addr)->sin_addr));
    u_short port_num_client = ntohs(((struct sockaddr_in *) &client_addr)->sin_port);
    printf("%s (%s:%i) is now connected to you\n", file_sender, ip_addr_client, port_num_client);

    /* receiving file */
    printf("Receiving the file...\n");

    /* receiving a first structure to check if file exists on client's computer and if yes, with the size of the file */
    if (recv(server_p2p->fd_socket, &server_p2p->packet.header, sizeof(struct Header), MSG_WAITALL) <= 0) {
        perror("recv FILE_SEND");
        exit(EXIT_FAILURE);
    }

    /* if name of the file to send did not exist on the client's computer
     * file transfer is aborted */
    if (server_p2p->packet.header.type == FILENAME) {
        printf("Error: file not sent\n");
        recv(server_p2p->fd_socket, server_p2p->buffer, 0, MSG_WAITALL);
        close(server_p2p->fd_socket);
        printf("Connection closed with %s (%s:%hu)\n", file_sender, ip_addr_client, port_num_client);
        free(server_p2p);
        return;
    }

    /* receiving the size of the file */
    if (recv(server_p2p->fd_socket, server_p2p->buffer, server_p2p->packet.header.len_payload, MSG_WAITALL) <= 0) {
        perror("recv file size");
        exit(EXIT_FAILURE);
    }
    /* receiving data of the file from client and saving it into a new file on our computer */
    /* creating directory where the received file will be stored */
    if (mkdir("inbox", S_IRWXU | S_IRWXG | S_IRWXO) && errno != EEXIST) {
        perror("mkdir:");
    }
    char filename_dest[2 * NICK_LEN];
    /* creating the path of the received file */
    sprintf(filename_dest, "inbox/%s", client->file_to_receive);
    /* opening the file */
    int fd_new_file = open(filename_dest, O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
    if (fd_new_file == -1) {
        perror("creating file");
        exit(EXIT_FAILURE);
    }
    long size = strtol(server_p2p->buffer, NULL, 10);
    long ret = 0, offset = 0;
    /* while we did not receive all the data of the file */
    while (offset != size) {
        recv(server_p2p->fd_socket, &server_p2p->packet.header, sizeof(struct Header), MSG_WAITALL);
        /* receiving file by frame of max 1024 bytes */
        recv(server_p2p->fd_socket, server_p2p->buffer, server_p2p->packet.header.len_payload, MSG_WAITALL);
        /* writing received data in a new file */
        if (-1 == (ret = write(fd_new_file, server_p2p->buffer, server_p2p->packet.header.len_payload))) {
            perror("Writing in new file");
        }
        offset += ret;
        /* progress bar to show the advancement of downloading */
        char progress_bar[12] = {'[', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', ']'};
        int progression = (int) (((double) offset / (double) size) * 100);
        for (int j = 1; j <= progression / 10; j++) {
            progress_bar[j] = '#';
        }
        for (int j = 0; j < (int) (sizeof(progress_bar) / sizeof(char)); j++) {
            printf("%c", progress_bar[j]);
        }
        printf(" [%i%%]\n", progression);
        printf("\033[1A\33[2K\r");
    }
    printf("File received successfully and saved in %s\n", filename_dest);

    server_p2p->packet.header.type = FILE_ACK;
    /* sending ack */
    send(server_p2p->fd_socket, &server_p2p->packet.header, sizeof(struct Header), 0);
    recv(server_p2p->fd_socket, server_p2p->buffer, 0, MSG_WAITALL);
    close(server_p2p->fd_socket);
    printf("Connection closed with %s\n", file_sender);
    free(server_p2p);
    return;
}

void file_reject(struct Client *client) {
    memset(client->file_to_send, 0, NICK_LEN);
    packet_set(&client->packet, client->nickname, FILE_REJECT, client->packet.header.infos, client->buffer);
    /* Sending structure and payload */
    packet_send(&client->packet,  client->fd_socket);
    printf("You rejected the file transfer\n");
}

void file_requestFromServer(struct Client *client) {
    strcpy(client->file_to_receive, client->buffer);
    printf("[%s]: %s wants you to accept the transfer of the file named \"%s\". Do you accept ? [Y/N]\n", client->packet.header.from, client->packet.header.infos, client->file_to_receive);

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
            file_accept_from_stdin(client, client->packet.header.infos);
            return;
        } else if (strcmp(client->buffer, "N") == 0 || strcmp(client->buffer, "n") == 0) {
            file_reject(client);
            return;
        } else {
            printf("--> Please only Y or N !\n");
        }
    }
}

/* Connecting to server socket */
struct Client *client_init(char *hostname, char *port) {
    struct Client *client = malloc(sizeof(struct Client));
    memset(client, 0, sizeof(struct Client));
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(hostname, port, &hints, &result) != 0) {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        client->fd_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (client->fd_socket == -1) {
            continue;
        }
        if (connect(client->fd_socket, rp->ai_addr, rp->ai_addrlen) != -1) {
            /* getting ip address and port of connection */
            struct sockaddr_in *sockaddr_in_ptr = (struct sockaddr_in *) rp->ai_addr;
            socklen_t len = sizeof(struct sockaddr_in);
            getsockname(client->fd_socket, (struct sockaddr *) sockaddr_in_ptr, &len);
            client->port_num = ntohs(sockaddr_in_ptr->sin_port);
            strcpy(client->ip_addr, inet_ntoa(sockaddr_in_ptr->sin_addr));
            break;
        }
        close(client->fd_socket);
    }
    if (rp == NULL) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(result);
    printf("You (%s:%hu) are now connected to the server (%s:%s)\n", client->ip_addr, client->port_num, hostname, port);
    return client;
}

/* connecting to the server and sending file */
void file_accept_from_server(struct Client *client) {
    printf("[SERVER]: %s accepted file transfer\n", client->packet.header.from);
    /* getting ip address and port to connect to the server */
    char *ip_addr_server = strtok(client->buffer, ":");
    char *port_num_server = strtok(NULL, "\n");
    /* connecting to the server */
    struct Client *client_p2p = client_init(ip_addr_server, port_num_server);
    /* sending file */
    printf("Sending the file...\n");
    struct stat sb;
    memset(&sb, 0, sizeof(struct stat));
    stat(client->file_to_send, &sb);
    if (!S_ISREG(sb.st_mode)) {
        printf("Invalid filename\n");
        client_p2p->packet.header.type = FILENAME;
        send(client_p2p->fd_socket, &client_p2p->packet.header, sizeof(struct Header), 0);
        close(client_p2p->fd_socket);
        free(client_p2p);
        printf("Connection closed with %s\n", client->packet.header.from);
        return;
    }
    /* opening file to send */
    int fileFd = open(client->file_to_send, O_RDONLY);
    if (fileFd == -1) {
        if (errno == ENOENT) {
            printf("Invalid filename\n");
            client_p2p->packet.header.type = FILENAME;
            send(client_p2p->fd_socket, &client_p2p->packet.header, sizeof(struct Header), 0);
            close(client_p2p->fd_socket);
            printf("Connection closed with %s\n", client->packet.header.from);
            free(client_p2p);
            return;
        }
        perror("Error opening file");
        exit(EXIT_FAILURE);

    }
    /* getting the size of the file to send */
    long size = sb.st_size;
    sprintf(client_p2p->packet.payload, "%lu", size);
    /* sending the size of the file to be sent */
    packet_set(&client_p2p->packet, client->nickname, FILE_SEND, "\0", client_p2p->packet.payload);
    packet_send(&client_p2p->packet, client_p2p->fd_socket);
    memset(client_p2p->packet.payload, 0, MSG_LEN);

    long offset = 0;
    /* while file not totally sent */
    /* sending file by frames of 1024 Bytes */
    while (offset != size) {
        /* putting frame of 1024 Bytes in payload */
        long ret = read(fileFd, client_p2p->packet.payload, MSG_LEN);
        if (-1 == ret) {
            perror("Reading from client socket");
            exit(EXIT_FAILURE);
        }
        if (0 == ret) {
            printf("Should close connection, read 0 bytes\n");
            close(fileFd);
            break;
        }
        client_p2p->packet.header.len_payload = ret;
        packet_send(&client_p2p->packet, client_p2p->fd_socket);
        /* waiting data to be read by dest user in the socket file*/
        usleep(1000);
        offset += ret;
        /* displaying progression bar while sending file */
        char progress_bar[12] = {'[', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', ']'}; //barre de progression de l'envoi du fichier
        int progression = (int) (((double) offset / (double) size) * 100);
        for (int j = 1; j <= progression / 10; j++) {
            progress_bar[j] = '#';
        }
        for (int j = 0; j < (int) (sizeof(progress_bar) / sizeof(char)); j++) {
            printf("%c", progress_bar[j]);
        }
        printf(" [%i%%]\n", progression);
        printf("\033[1A\33[2K\r");
    }
    printf("File sent successfully\n");

    /* receiving ack */
    recv(client_p2p->fd_socket, &client_p2p->packet.header, sizeof(struct Header), MSG_WAITALL);
    if (client_p2p->packet.header.type == FILE_ACK) {
        printf("%s has received the file\n", client->packet.header.from);
    } else {
        printf("Problem in reception\n");
    }
    /* closing connection */
    close(client_p2p->fd_socket);
    free(client_p2p);
    printf("Connection closed with %s\n", client->packet.header.from);
}

/* processing data coming from the server */
int from_server(struct Client *client) {
    /*Receiving structure*/
    if (recv(client->fd_socket, &client->packet.header, sizeof(struct Header), MSG_WAITALL) <= 0) {
        printf("Server has crashed\n");
        return 0;
    }
    /* Receiving message*/
    if (client->packet.header.len_payload != 0 && recv(client->fd_socket, client->buffer, client->packet.header.len_payload, MSG_WAITALL) <= 0) {
        perror("recv");
        return 0;
    }

    switch (client->packet.header.type) {
        /* changing nickname */
        case NICKNAME_NEW:
            nickname_new_from_server(client);
            break;
            /* if receiving a file request */
        case FILE_REQUEST:
            file_requestFromServer(client);
            return 1;
            /* if receiving a file accept */
        case FILE_ACCEPT:
            file_accept_from_server(client);
            return 1;
        default:
            break;
    }

    printf("[%s]: %s\n", client->packet.header.from, client->buffer);
    return 1;
}

/* disconnecting client from server */
void disconnect_from_server(struct pollfd *pollfds) {
    for (int j = 0; j < NUM_FDS; j++) {
        close(pollfds[j].fd);
        pollfds[j].fd = -1;
        pollfds[j].events = 0;
        pollfds[j].revents = 0;
    }
    printf("You are disconnected\n");
}

int run_client(struct Client *client) {
    /* Declare array of struct pollfd */
    struct pollfd pollfds[NUM_FDS];
    /* Init first slots with listening socket  to receive data */
    /* from server */
    pollfds[0].fd = client->fd_socket;
    pollfds[0].events = POLLIN;
    pollfds[0].revents = 0;
    /* from keyboard */
    pollfds[1].fd = STDIN_FILENO;
    pollfds[1].events = POLLIN;
    pollfds[1].revents = 0;
    /* client loop */
    while (1) {
        fflush(stdout);
        if (-1 == poll(pollfds, NUM_FDS, -1)) {
            perror("Poll");
        }
        for (int i = 0; i < NUM_FDS; i++) {
            memset(&client->packet.header, 0, sizeof(struct Header));
            memset(client->buffer, 0, MSG_LEN);
            memset(client->packet.payload, 0, MSG_LEN);
            /* if data comes from keyboard */
            if (pollfds[i].fd == STDIN_FILENO && pollfds[i].revents & POLLIN) {
                strcpy(client->packet.header.from, client->nickname);
                if (!from_stdin(client)) {
                    disconnect_from_server(pollfds);
                    return 1;
                }
                pollfds[i].revents = 0;
            } /* if data comes from server */
            else if (pollfds[i].fd != STDIN_FILENO && pollfds[i].revents & POLLIN) {
                if (!from_server(client)) {
                    disconnect_from_server(pollfds);
                    return 0;
                }
                pollfds[i].revents = 0;
            }
        }
    }
}

void usage() {
    printf("Usage: ./client hostname port_number\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        usage();
    }

    struct Client *client = client_init(argv[1], argv[2]);
    /* running client */
    if (!run_client(client)) {
        free(client);
        exit(EXIT_FAILURE);
    }

    free(client);
    exit(EXIT_SUCCESS);
}
