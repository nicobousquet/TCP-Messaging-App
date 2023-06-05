#include "../include/client.h"
#include "../include/peer.h"
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void nickname_new_req(struct Client *client, char *new_nickname) {
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
    packet_send(&client->packet,  client->socket_fd);
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

void nickname_list_req(struct Client *client) {
    packet_set(&client->packet, client->nickname, NICKNAME_LIST, "\0", "\0");
    packet_send(&client->packet,  client->socket_fd);
    return;
}

void nickname_infos_req(struct Client *client, char *nickname) {
    packet_set(&client->packet, client->nickname, NICKNAME_INFOS, nickname, "\0");
    packet_send(&client->packet,  client->socket_fd);
    return;
}

void broadcast_send_req(struct Client *client, char *payload) {
    packet_set(&client->packet, client->nickname, BROADCAST_SEND, "\0", payload);
    packet_send(&client->packet,  client->socket_fd);
    return;
}

void unicast_send_req(struct Client *client, char *nickname_dest, char *payload) {
    /* checking dest user and payload are corrects */
    if (nickname_dest == NULL || payload == NULL) {
        printf("Invalid arguments\n");
        return;
    }

    packet_set(&client->packet, client->nickname, UNICAST_SEND, nickname_dest, payload);
    packet_send(&client->packet,  client->socket_fd);
    return;
}

void multicast_create_req(struct Client *client, char *name_channel) {
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
    packet_send(&client->packet,  client->socket_fd);
    return;
}

void multicast_list_req(struct Client *client) {
    packet_set(&client->packet, client->nickname, MULTICAST_LIST, "\0", "\0");
    packet_send(&client->packet,  client->socket_fd);
    return;
}

void multicast_join_req(struct Client *client, char *chatroom) {
    packet_set(&client->packet, client->nickname, MULTICAST_JOIN, chatroom, "\0");
    packet_send(&client->packet,  client->socket_fd);
    return;
}

int quit_req(struct Client *client, char *name_channel) {
    /* if channel name exists, quitting the channel */
    if (name_channel == NULL) {
        /* quitting the server */
        return 0;
    }
    packet_set(&client->packet, client->nickname, MULTICAST_QUIT, name_channel, "\0");
    packet_send(&client->packet,  client->socket_fd);
    return 1;
}

/* extracting name of file to send */
static void extract_filename(char *tmp, char *sub_path) {
    tmp = strtok(tmp, "/");
    strcpy(sub_path, tmp);
    /* extracting the name of the file from the whole path */
    while ((tmp = strtok(NULL, "/")) != NULL) {
        memset(sub_path, 0, NICK_LEN);
        strcpy(sub_path, tmp);
    }
}

void file_req(struct Client *client, char *nickname_dest, char *filename) {
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
    packet_send(&client->packet,  client->socket_fd);
    /* Sending structure and payload */
    printf("File request sent to %s\n", nickname_dest);
    return;
}

void multicast_send_req(struct Client *client, char *phrase, char *first_word) {
    if (phrase != NULL) {
        sprintf(client->packet.payload, "%s %s", first_word, phrase);
    } else {
        strcpy(client->packet.payload, first_word);
    }

    packet_set(&client->packet, client->nickname, MULTICAST_SEND, "\0", client->packet.payload);
    packet_send(&client->packet,  client->socket_fd);
    return;
}

/* processing data entered from keyboard */
int handle_reqs(struct Client *client) {
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
        nickname_new_req(client, strtok(NULL, " "));
        return 1;
    } else if (strcmp(first_word, "/help") == 0) {
        help();
        return 1;
    } else if (strcmp(first_word, "/who") == 0) {
        nickname_list_req(client);
        return 1;
    } else if (strcmp(first_word, "/whois") == 0) {
        nickname_infos_req(client, strtok(NULL, ""));
        return 1;
    } else if (strcmp(first_word, "/msgall") == 0) {
        broadcast_send_req(client, strtok(NULL, ""));
        return 1;
    } else if (strcmp(first_word, "/msg") == 0) {
        char *nickname_dest = strtok(NULL, " ");
        char *payload = strtok(NULL, "");
        unicast_send_req(client, nickname_dest, payload);
        return 1;
    } else if (strcmp(first_word, "/create") == 0) {
        multicast_create_req(client, strtok(NULL, ""));
        return 1;
    } else if (strcmp(first_word, "/channel_list") == 0) {
        multicast_list_req(client);
        return 1;
    } else if (strcmp(first_word, "/join") == 0) {
        multicast_join_req(client, strtok(NULL, ""));
        return 1;
    } else if (strcmp(first_word, "/quit") == 0) {
        if (!quit_req(client, strtok(NULL, ""))) {
            return 0;
        }
        return 1;
    } else if (strcmp(first_word, "/send") == 0) {
        char *nickname_dest = strtok(NULL, " ");
        char *filename = strtok(NULL, "");
        file_req(client, nickname_dest, filename);
        return 1;
    } else {
        multicast_send_req(client, strtok(NULL, ""), first_word);
        return 1;
    }
}

void nickname_new_res(struct Client *client) {
    strcpy(client->nickname, client->packet.header.infos);
}

static void file_accept_req(struct Client *client, char *file_sender) {
    printf("You accepted the file transfer\n");
    /* letting the computer choosing a listening port */
    char listening_port[INFOS_LEN] = "0";
    /* creating a listening socket */
    struct Peer *peer_dest = peer_init_peer_dest(listening_port, client->nickname);
    sprintf(client->packet.payload, "%s:%hu", peer_dest->ip_addr, peer_dest->port_num);
    /* sending ip address and port for the client to connect */
    packet_set(&client->packet, client->nickname, FILE_ACCEPT, file_sender, client->packet.payload);
    packet_send(&client->packet, client->socket_fd);

    struct sockaddr peer_dest_addr;
    memset(&peer_dest_addr, 0, sizeof(peer_dest_addr));
    socklen_t len = sizeof(peer_dest_addr);
    /* accepting connection from client */
    if ((peer_dest->socket_fd = accept(peer_dest->socket_fd, &peer_dest_addr, &len)) == -1) {
        perror("Accept");
    }
    getpeername(peer_dest->socket_fd, (struct sockaddr *) &peer_dest_addr, &len);
    char ip_addr_client[INFOS_LEN];
    strcpy(ip_addr_client, inet_ntoa(((struct sockaddr_in *) &peer_dest_addr)->sin_addr));
    u_short port_num_client = ntohs(((struct sockaddr_in *) &peer_dest_addr)->sin_port);
    printf("%s (%s:%i) is now connected to you\n", file_sender, ip_addr_client, port_num_client);

    if (!peer_receive_file(peer_dest, client->file_to_receive)) {
        printf("Error: file not sent\n");
        recv(peer_dest->socket_fd, peer_dest->buffer, 0, MSG_WAITALL);
        close(peer_dest->socket_fd);
        printf("Connection closed with %s (%s:%hu)\n", file_sender, ip_addr_client, port_num_client);
        free(peer_dest);
    }

    recv(peer_dest->socket_fd, peer_dest->buffer, 0, MSG_WAITALL);
    close(peer_dest->socket_fd);
    printf("Connection closed with %s\n", file_sender);
    free(peer_dest);
    return;
}

static void file_reject_req(struct Client *client) {
    memset(client->file_to_send, 0, NICK_LEN);
    packet_set(&client->packet, client->nickname, FILE_REJECT, client->packet.header.infos, client->buffer);
    /* Sending structure and payload */
    packet_send(&client->packet,  client->socket_fd);
    printf("You rejected the file transfer\n");
}

void file_request_res(struct Client *client) {
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
            file_accept_req(client, client->packet.header.infos);
            return;
        } else if (strcmp(client->buffer, "N") == 0 || strcmp(client->buffer, "n") == 0) {
            file_reject_req(client);
            return;
        } else {
            printf("--> Please only Y or N !\n");
        }
    }
}

/* connecting to the server and sending file */
void file_accept_res(struct Client *client) {
    printf("[SERVER]: %s accepted file transfer\n", client->packet.header.from);
    /* getting ip address and port to connect to the server */
    char *ip_addr_server = strtok(client->buffer, ":");
    char *port_num_server = strtok(NULL, "\n");
    /* connecting to the server */
    struct Peer *peer_src = peer_init_peer_src(ip_addr_server, port_num_server);

    if (!peer_send_file(peer_src, client->file_to_send)) {
        printf("Invalid filename\n");
        packet_set(&peer_src->packet, peer_src->nickname, FILENAME, "", "");
        packet_send(&peer_src->packet, peer_src->socket_fd);
        close(peer_src->socket_fd);
        free(peer_src);
        printf("Connection closed with %s\n", client->packet.header.from);
    }

    /* receiving ack */
    recv(peer_src->socket_fd, &peer_src->packet.header, sizeof(struct Header), MSG_WAITALL);

    if (peer_src->packet.header.type == FILE_ACK) {
        printf("%s has received the file\n", client->packet.header.from);
    } else {
        printf("Problem in reception\n");
    }
    /* closing connection */
    close(peer_src->socket_fd);
    free(peer_src);
    printf("Connection closed with %s\n", client->packet.header.from);
}

/* processing data coming from the server */
int handle_res(struct Client *client) {
    /*Receiving structure*/
    if (recv(client->socket_fd, &client->packet.header, sizeof(struct Header), MSG_WAITALL) <= 0) {
        printf("Server has crashed\n");
        return 0;
    }
    /* Receiving message*/
    if (client->packet.header.len_payload != 0 && recv(client->socket_fd, client->buffer, client->packet.header.len_payload, MSG_WAITALL) <= 0) {
        perror("recv");
    }

    switch (client->packet.header.type) {
        /* changing nickname */
        case NICKNAME_NEW:
            nickname_new_res(client);
            break;
            /* if receiving a file request */
        case FILE_REQUEST:
            file_request_res(client);
            return 1;
            /* if receiving a file accept */
        case FILE_ACCEPT:
            file_accept_res(client);
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
    pollfds[0].fd = client->socket_fd;
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
                if (!handle_reqs(client)) {
                    disconnect_from_server(pollfds);
                    return 1;
                }
                pollfds[i].revents = 0;
            } /* if data comes from server */
            else if (pollfds[i].fd != STDIN_FILENO && pollfds[i].revents & POLLIN) {
                if (!handle_res(client)) {
                    disconnect_from_server(pollfds);
                    return 0;
                }
                pollfds[i].revents = 0;
            }
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
        client->socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (client->socket_fd == -1) {
            continue;
        }
        if (connect(client->socket_fd, rp->ai_addr, rp->ai_addrlen) != -1) {
            /* getting ip address and port of connection */
            struct sockaddr_in *sockaddr_in_ptr = (struct sockaddr_in *) rp->ai_addr;
            socklen_t len = sizeof(struct sockaddr_in);
            getsockname(client->socket_fd, (struct sockaddr *) sockaddr_in_ptr, &len);
            client->port_num = ntohs(sockaddr_in_ptr->sin_port);
            strcpy(client->ip_addr, inet_ntoa(sockaddr_in_ptr->sin_addr));
            break;
        }
        close(client->socket_fd);
    }
    if (rp == NULL) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(result);
    printf("You (%s:%hu) are now connected to the server (%s:%s)\n", client->ip_addr, client->port_num, hostname, port);
    return client;
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
