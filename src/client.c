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

void client_send_nickname_new_req(struct client *client, char *new_nickname) {
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
    packet_send(&client->packet, client->socket_fd);
}

void client_help() {
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

void client_send_nickname_list_req(struct client *client) {
    packet_set(&client->packet, client->nickname, NICKNAME_LIST, "\0", "\0");
    packet_send(&client->packet, client->socket_fd);
}

void client_send_nickname_infos_req(struct client *client, char *nickname) {
    packet_set(&client->packet, client->nickname, NICKNAME_INFOS, nickname, "\0");
    packet_send(&client->packet, client->socket_fd);
}

void client_send_broadcast_send_req(struct client *client, char *payload) {
    packet_set(&client->packet, client->nickname, BROADCAST_SEND, "\0", payload);
    packet_send(&client->packet, client->socket_fd);
}

void client_send_unicast_send_req(struct client *client, char *nickname_dest, char *payload) {
    /* checking dest user and payload are corrects */
    if (nickname_dest == NULL || payload == NULL) {
        printf("Invalid arguments\n");
        return;
    }

    packet_set(&client->packet, client->nickname, UNICAST_SEND, nickname_dest, payload);
    packet_send(&client->packet, client->socket_fd);
}

void client_send_multicast_create_req(struct client *client, char *name_channel) {
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
    packet_send(&client->packet, client->socket_fd);
}

void client_send_multicast_list_req(struct client *client) {
    packet_set(&client->packet, client->nickname, MULTICAST_LIST, "\0", "\0");
    packet_send(&client->packet, client->socket_fd);
}

void client_send_multicast_join_req(struct client *client, char *chatroom) {
    packet_set(&client->packet, client->nickname, MULTICAST_JOIN, chatroom, "\0");
    packet_send(&client->packet, client->socket_fd);
}

void client_send_multicast_quit_req(struct client *client, char *name_channel) {
    /* if channel name exists, quitting the channel */
    packet_set(&client->packet, client->nickname, MULTICAST_QUIT, name_channel, "\0");
    packet_send(&client->packet, client->socket_fd);
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

void client_send_file_req(struct client *client, char *nickname_dest, char *filename) {
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
    packet_send(&client->packet, client->socket_fd);
    /* Sending structure and payload */
    printf("File request sent to %s\n", nickname_dest);
}

void client_send_multicast_send_req(struct client *client, char *phrase, char *first_word) {
    if (phrase != NULL) {
        sprintf(client->packet.payload, "%s %s", first_word, phrase);
    } else {
        strcpy(client->packet.payload, first_word);
    }

    packet_set(&client->packet, client->nickname, MULTICAST_SEND, "\0", client->packet.payload);
    packet_send(&client->packet, client->socket_fd);
}

void client_handle_nickname_new_res(struct client *client) {
    strcpy(client->nickname, client->packet.header.infos);
}

static void file_accept_req(struct client *client, char *file_sender) {
    printf("You accepted the file transfer\n");
    /* letting the computer choosing a listening port */
    char listening_port[INFOS_LEN] = "0";
    /* creating a listening socket */
    struct peer *peer_dest = peer_init_peer_dest(listening_port, client->nickname);
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
        return;
    }

    recv(peer_dest->socket_fd, peer_dest->buffer, 0, MSG_WAITALL);
    close(peer_dest->socket_fd);
    printf("Connection closed with %s\n", file_sender);
    free(peer_dest);
}

static void file_reject_req(struct client *client) {
    memset(client->file_to_send, 0, NICK_LEN);
    packet_set(&client->packet, client->nickname, FILE_REJECT, client->packet.header.infos, client->buffer);
    /* Sending structure and payload */
    packet_send(&client->packet, client->socket_fd);
    printf("You rejected the file transfer\n");
}

void client_handle_file_request_res(struct client *client) {
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
void client_handle_file_accept_res(struct client *client) {
    printf("[SERVER]: %s accepted file transfer\n", client->packet.header.from);
    /* getting ip address and port to connect to the server */
    char *ip_addr_server = strtok(client->buffer, ":");
    char *port_num_server = strtok(NULL, "\n");
    /* connecting to the server */
    struct peer *peer_src = peer_init_peer_src(ip_addr_server, port_num_server);

    if (!peer_send_file(peer_src, client->file_to_send)) {
        printf("Invalid filename\n");
        packet_set(&peer_src->packet, peer_src->nickname, FILENAME, "", "");
        packet_send(&peer_src->packet, peer_src->socket_fd);
        close(peer_src->socket_fd);
        free(peer_src);
        printf("Connection closed with %s\n", client->packet.header.from);
        return;
    }
    /* receiving ack */
    recv(peer_src->socket_fd, &peer_src->packet.header, sizeof(struct header), MSG_WAITALL);

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

/* disconnecting client from server */
void client_disconnect_from_server(struct pollfd *pollfds) {
    for (int j = 0; j < NUM_FDS; j++) {
        close(pollfds[j].fd);
        pollfds[j].fd = -1;
        pollfds[j].events = 0;
        pollfds[j].revents = 0;
    }
    printf("You are disconnected\n");
}

/* Connecting to server socket */
struct client *client_init(char *hostname, char *port) {
    struct client *client = malloc(sizeof(struct client));
    memset(client, 0, sizeof(struct client));
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