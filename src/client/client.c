#include "../../include/client/client.h"
#include "../../include/client/peer.h"
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Connecting to server socket */
struct client client_init(char *hostname, char *port) {
    struct client client;
    memset(&client, 0, sizeof(struct client));
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, port, &hints, &result) != 0) {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        client.socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (client.socket_fd == -1) {
            continue;
        }

        if (connect(client.socket_fd, rp->ai_addr, rp->ai_addrlen) != -1) {
            /* getting ip address and port of connection */
            struct sockaddr_in *sockaddr_in_ptr = (struct sockaddr_in *) rp->ai_addr;
            socklen_t len = sizeof(struct sockaddr_in);
            getsockname(client.socket_fd, (struct sockaddr *) sockaddr_in_ptr, &len);
            client.port_num = ntohs(sockaddr_in_ptr->sin_port);
            inet_ntop(AF_INET, &sockaddr_in_ptr->sin_addr, client.ip_addr, INET_ADDRSTRLEN);
            break;
        }

        close(client.socket_fd);
    }

    if (rp == NULL) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);
    printf("You (%s:%hu) are now connected to the server (%s:%s)\n", client.ip_addr, client.port_num, hostname, port);

    return client;
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

void client_send_nickname_new_req(struct client *client, char *new_nickname) {
    if (new_nickname == NULL) {
        printf("Please, choose a nickname !\n");
        return;
    }

    unsigned long len_nickname = strlen(new_nickname);

    /* checking length */
    unsigned long len_correct_nickname = strspn(new_nickname, "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm1234567890");

    if (len_nickname > NICK_LEN - 1) {
        printf("Too long nickname\n");
        return;
    }

    /* checking types of characters */
    if (len_correct_nickname != len_nickname) {
        printf("Nickname with non authorized characters, please, only numbers and letters\n");
        return;
    }

    struct packet req_packet = packet_init(client->nickname, NICKNAME_NEW, new_nickname, "", 0);
    packet_send(&req_packet, client->socket_fd);
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
    struct packet req_packet = packet_init(client->nickname, NICKNAME_LIST, "", "", 0);
    packet_send(&req_packet, client->socket_fd);
}

void client_send_nickname_infos_req(struct client *client, char *nickname) {
    struct packet req_packet = packet_init(client->nickname, NICKNAME_INFOS, nickname, "", 0);
    packet_send(&req_packet, client->socket_fd);
}

void client_send_broadcast_send_req(struct client *client, char *payload) {
    struct packet req_packet = packet_init(client->nickname, BROADCAST_SEND, "", payload, strlen(payload));
    packet_send(&req_packet, client->socket_fd);
}

void client_send_unicast_send_req(struct client *client, char *nickname_dest, char *payload) {
    /* checking dest user and payload are corrects */
    if (nickname_dest == NULL || payload == NULL) {
        printf("Invalid arguments\n");
        return;
    }

    struct packet req_packet = packet_init(client->nickname, UNICAST_SEND, nickname_dest, payload, strlen(payload));
    packet_send(&req_packet, client->socket_fd);
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

    struct packet req_packet = packet_init(client->nickname, MULTICAST_CREATE, name_channel, "", 0);
    packet_send(&req_packet, client->socket_fd);
}

void client_send_multicast_list_req(struct client *client) {
    struct packet req_packet = packet_init(client->nickname, MULTICAST_LIST, "", "", 0);
    packet_send(&req_packet, client->socket_fd);
}

void client_send_multicast_join_req(struct client *client, char *chatroom) {
    struct packet req_packet = packet_init(client->nickname, MULTICAST_JOIN, chatroom, "", 0);
    packet_send(&req_packet, client->socket_fd);
}

void client_send_multicast_quit_req(struct client *client, char *name_channel) {
    /* if channel name exists, quitting the channel */
    struct packet req_packet = packet_init(client->nickname, MULTICAST_QUIT, name_channel, "", 0);
    packet_send(&req_packet, client->socket_fd);
}

/* extracting name of file to send */
static void extract_filename(char *tmp, char *sub_path) {
    tmp = strtok(tmp, "/");
    snprintf(sub_path, FILENAME_LEN, "%s", tmp);

    /* extracting the name of the file from the whole path */
    while ((tmp = strtok(NULL, "/")) != NULL) {
        memset(sub_path, 0, FILENAME_LEN);
        snprintf(sub_path, FILENAME_LEN, "%s", tmp);
    }
}

void client_send_file_req(struct client *client, char *nickname_dest, char *filename) {
    /* getting path of file to send */
    if (nickname_dest == NULL || filename == NULL) {
        printf("Invalid arguments\n");
        return;
    }

    snprintf(client->file_to_send, FILENAME_LEN, "%s", filename + 1);
    client->file_to_send[strlen(client->file_to_send) - 1] = '\0';

    /* extracting name of file to send from the path */
    char tmp[FILENAME_LEN];
    snprintf(tmp, FILENAME_LEN, "%s", client->file_to_send);
    char file[FILENAME_LEN];

    /* writing only name of file to send and not the whole path into the payload */
    extract_filename(tmp, file);

    struct packet req_packet = packet_init(client->nickname, FILE_REQUEST, nickname_dest, file, strlen(file));
    packet_send(&req_packet, client->socket_fd);

    /* Sending structure and payload */
    printf("File request sent to %s\n", nickname_dest);
}

void client_send_multicast_send_req(struct client *client, char *sentence, char *first_word) {
    if (sentence != NULL) {
        char payload[MSG_LEN];

        snprintf(payload, MSG_LEN, "%s %s", first_word, sentence);

        struct packet req_packet = packet_init(client->nickname, MULTICAST_SEND, "", payload, strlen(payload));
        packet_send(&req_packet, client->socket_fd);
    } else {
        struct packet req_packet = packet_init(client->nickname, MULTICAST_SEND, "", first_word, strlen(first_word));
        packet_send(&req_packet, client->socket_fd);
    }
}

void client_handle_nickname_new_res(struct client *client, struct packet *res_packet) {
    snprintf(client->nickname, NICK_LEN, "%s", res_packet->header.infos);
}

static void file_accept_res(struct client *client, char *file_sender) {
    printf("You accepted the file transfer\n");

    /* creating a listening socket */
    struct peer peer_dest = peer_init_peer_dest(client->ip_addr, "0", client->nickname);

    char payload[MSG_LEN];
    snprintf(payload, MSG_LEN, "%s:%hu", peer_dest.ip_addr, peer_dest.port_num);

    /* sending ip address and port for the client to connect */
    struct packet res_packet = packet_init(client->nickname, FILE_ACCEPT, file_sender, payload, strlen(payload));
    packet_send(&res_packet, client->socket_fd);

    struct sockaddr sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    socklen_t len = sizeof(sockaddr);

    /* accepting connection from client */
    if ((peer_dest.socket_fd = accept(peer_dest.socket_fd, &sockaddr, &len)) == -1) {
        perror("Accept");
    }

    char ip_addr_client[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &((struct sockaddr_in *) &sockaddr)->sin_addr, ip_addr_client, INET_ADDRSTRLEN);
    u_short port_num_client = ntohs(((struct sockaddr_in *) &sockaddr)->sin_port);
    printf("%s (%s:%i) is now connected to you (%s:%hu)\n", file_sender, ip_addr_client, port_num_client, peer_dest.ip_addr, peer_dest.port_num);

    if (!peer_receive_file(&peer_dest, client->file_to_receive)) {
        printf("Error: file not sent\n");

        recv(peer_dest.socket_fd, payload, 0, MSG_WAITALL);
        close(peer_dest.socket_fd);
        printf("Connection closed with %s (%s:%hu)\n", file_sender, ip_addr_client, port_num_client);
        return;
    }

    recv(peer_dest.socket_fd, payload, 0, MSG_WAITALL);
    close(peer_dest.socket_fd);

    printf("Connection closed with %s\n", file_sender);
}

static void file_reject_res(struct client *client, char *file_sender) {
    memset(client->file_to_send, 0, FILENAME_LEN);

    struct packet res_packet = packet_init(client->nickname, FILE_REJECT, file_sender, "", 0);
    packet_send(&res_packet, client->socket_fd);

    printf("You rejected the file transfer\n");
}

void client_handle_file_request_res(struct client *client, struct packet *req_packet) {
    int len = snprintf(client->file_to_receive, FILENAME_LEN, "%s", req_packet->payload);

    if (len >= FILENAME_LEN) {
        printf("String truncated\n");
    }

    printf("[%s]: %s wants to send you the file named \"%s\". Do you accept ? [Y/N]\n", req_packet->header.from, req_packet->header.infos, client->file_to_receive);

    /* loop waiting for user response for file request */
    while (1) {
        char buffer[MSG_LEN];
        memset(buffer, 0, MSG_LEN);

        int n = 0;
        char c;

        while ((c = (char) getchar()) != '\n' && n != MSG_LEN) {
            buffer[n++] = c;
        }

        /* move to the beginning of previous line */
        printf("\033[1A\33[2K\r");
        fflush(stdout);

        if (strcmp(buffer, "Y") == 0 || strcmp(buffer, "y") == 0) {
            file_accept_res(client, req_packet->header.infos);
            return;
        } else if (strcmp(buffer, "N") == 0 || strcmp(buffer, "n") == 0) {
            file_reject_res(client, req_packet->header.infos);
            return;
        } else {
            printf("--> Please only Y or N !\n");
        }
    }
}

/* connecting to the server and sending file */
void client_handle_file_accept_res(struct client *client, struct packet *res_packet) {
    printf("[SERVER]: %s accepted file transfer\n", res_packet->header.from);

    /* getting ip address and port to connect to the server */
    char *ip_addr_peer_dest = strtok(res_packet->payload, ":");
    char *port_num_peer_dest = strtok(NULL, "\n");

    /* connecting to the server */
    struct peer peer_src = peer_init_peer_src(ip_addr_peer_dest, port_num_peer_dest, client->nickname);

    if (!peer_send_file(&peer_src, client->file_to_send)) {
        printf("Invalid filename\n");

        struct packet packet = packet_init(peer_src.nickname, FILENAME, "", "", 0);
        packet_send(&packet, peer_src.socket_fd);

        close(peer_src.socket_fd);
        printf("Connection closed with %s\n", res_packet->header.from);

        return;
    }

    /* closing connection */
    close(peer_src.socket_fd);
    printf("Connection closed with %s\n", res_packet->header.from);
}