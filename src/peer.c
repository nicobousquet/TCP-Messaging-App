#include "../include/peer.h"
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

struct peer *peer_init_peer_dest(char *listening_port, char *nickname_user) {
    struct peer *peer_dest = malloc(sizeof(struct peer));
    memset(peer_dest, 0, sizeof(struct peer));
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, listening_port, &hints, &result) != 0) {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        peer_dest->socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (peer_dest->socket_fd == -1) {
            continue;
        }
        if (bind(peer_dest->socket_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            /* getting ip address  and port*/
            struct sockaddr_in my_addr;
            socklen_t len = sizeof(my_addr);
            getsockname(peer_dest->socket_fd, (struct sockaddr *) &my_addr, &len);
            inet_ntop(AF_INET, &my_addr.sin_addr, peer_dest->ip_addr, 16);
            peer_dest->port_num = ntohs(my_addr.sin_port);
            strcpy(peer_dest->nickname, nickname_user);
            freeaddrinfo(result);
            if ((listen(peer_dest->socket_fd, SOMAXCONN)) != 0) {
                perror("listen()\n");
                exit(EXIT_FAILURE);
            }
            printf("Listening on %s:%hu\n", peer_dest->ip_addr, peer_dest->port_num);
            return peer_dest;
        }
        close(peer_dest->socket_fd);
    }
    perror("bind()");
    exit(EXIT_FAILURE);
}

int peer_receive_file(struct peer *peer_dest, char *file_to_receive) {
    /* receiving file */
    printf("Receiving the file...\n");

    /* receiving a first structure to check if file exists on client's computer and if yes, with the size of the file */
    if (recv(peer_dest->socket_fd, &peer_dest->packet.header, sizeof(struct header), MSG_WAITALL) <= 0) {
        perror("recv FILE_SEND");
        exit(EXIT_FAILURE);
    }

    /* if name of the file to send did not exist on the client's computer
     * file transfer is aborted */
    if (peer_dest->packet.header.type == FILENAME) {
        return 0;
    }

    /* receiving the size of the file */
    if (recv(peer_dest->socket_fd, peer_dest->buffer, peer_dest->packet.header.len_payload, MSG_WAITALL) <= 0) {
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
    sprintf(filename_dest, "inbox/%s", file_to_receive);
    /* opening the file */
    int fd_new_file = open(filename_dest, O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
    if (fd_new_file == -1) {
        perror("creating file");
        exit(EXIT_FAILURE);
    }
    long size = strtol(peer_dest->buffer, NULL, 10);
    long ret = 0, offset = 0;
    /* while we did not receive all the data of the file */
    while (offset != size) {
        recv(peer_dest->socket_fd, &peer_dest->packet.header, sizeof(struct header), MSG_WAITALL);
        /* receiving file by frame of max 1024 bytes */
        recv(peer_dest->socket_fd, peer_dest->buffer, peer_dest->packet.header.len_payload, MSG_WAITALL);
        /* writing received data in a new file */
        if (-1 == (ret = write(fd_new_file, peer_dest->buffer, peer_dest->packet.header.len_payload))) {
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

    packet_set(&peer_dest->packet, peer_dest->nickname, FILE_ACK, "", "");
    packet_send(&peer_dest->packet, peer_dest->socket_fd);

    return 1;
}

struct peer *peer_init_peer_src(char *hostname, char *port) {
    struct peer *peer_src = malloc(sizeof(struct peer));
    memset(peer_src, 0, sizeof(struct peer));
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(hostname, port, &hints, &result) != 0) {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        peer_src->socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (peer_src->socket_fd == -1) {
            continue;
        }
        if (connect(peer_src->socket_fd, rp->ai_addr, rp->ai_addrlen) != -1) {
            /* getting ip address and port of connection */
            struct sockaddr_in *sockaddr_in_ptr = (struct sockaddr_in *) rp->ai_addr;
            socklen_t len = sizeof(struct sockaddr_in);
            getsockname(peer_src->socket_fd, (struct sockaddr *) sockaddr_in_ptr, &len);
            peer_src->port_num = ntohs(sockaddr_in_ptr->sin_port);
            strcpy(peer_src->ip_addr, inet_ntoa(sockaddr_in_ptr->sin_addr));
            break;
        }
        close(peer_src->socket_fd);
    }
    if (rp == NULL) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(result);
    printf("You (%s:%hu) are now connected to the server (%s:%s)\n", peer_src->ip_addr, peer_src->port_num, hostname, port);
    return peer_src;
}

int peer_send_file(struct peer *peer_src, char *file_to_send) {
    /* sending file */
    printf("Sending the file...\n");
    struct stat sb;
    memset(&sb, 0, sizeof(struct stat));
    stat(file_to_send, &sb);
    if (!S_ISREG(sb.st_mode)) {
        return 0;
    }
    /* opening file to send */
    int fileFd = open(file_to_send, O_RDONLY);
    if (fileFd == -1) {
        if (errno == ENOENT) {
            return 0;
        }
        perror("Error opening file");
        exit(EXIT_FAILURE);

    }
    /* getting the size of the file to send */
    long size = sb.st_size;
    sprintf(peer_src->packet.payload, "%lu", size);
    /* sending the size of the file to be sent */
    packet_set(&peer_src->packet, peer_src->nickname, FILE_SEND, "\0", peer_src->packet.payload);
    packet_send(&peer_src->packet, peer_src->socket_fd);
    memset(peer_src->packet.payload, 0, MSG_LEN);

    long offset = 0;
    /* while file not totally sent */
    /* sending file by frames of 1024 Bytes */
    while (offset != size) {
        /* putting frame of 1024 Bytes in payload */
        long ret = read(fileFd, peer_src->packet.payload, MSG_LEN);
        if (-1 == ret) {
            perror("Reading from client socket");
            exit(EXIT_FAILURE);
        }
        if (0 == ret) {
            printf("Should close connection, read 0 bytes\n");
            close(fileFd);
            break;
        }
        peer_src->packet.header.len_payload = ret;
        packet_send(&peer_src->packet, peer_src->socket_fd);
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
    return 1;
}