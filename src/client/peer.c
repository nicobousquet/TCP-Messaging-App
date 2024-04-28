#include "../../include/client/peer.h"
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

struct peer peer_init_peer_dest(char *listening_addr, char *listening_port, char *nickname_user) {
    struct peer peer_dest;

    memset(&peer_dest, 0, sizeof(struct peer));

    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(listening_addr, listening_port, &hints, &result) != 0) {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        peer_dest.socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (peer_dest.socket_fd == -1) {
            continue;
        }

        if (bind(peer_dest.socket_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            /* getting ip address  and port*/
            struct sockaddr_in my_addr;
            socklen_t len = sizeof(my_addr);
            getsockname(peer_dest.socket_fd, (struct sockaddr *) &my_addr, &len);
            inet_ntop(AF_INET, &my_addr.sin_addr, peer_dest.ip_addr, INET_ADDRSTRLEN);
            peer_dest.port_num = ntohs(my_addr.sin_port);
            snprintf(peer_dest.nickname, NICK_LEN, "%s", nickname_user);
            freeaddrinfo(result);

            if ((listen(peer_dest.socket_fd, SOMAXCONN)) != 0) {
                perror("listen()\n");
                exit(EXIT_FAILURE);
            }

            printf("Listening on %s:%hu\n", peer_dest.ip_addr, peer_dest.port_num);

            return peer_dest;
        }

        close(peer_dest.socket_fd);
    }

    perror("bind()");
    exit(EXIT_FAILURE);
}

int peer_receive_file(struct peer *peer_dest, char *file_to_receive) {
    /* receiving file */
    printf("Receiving the file...\n");

    struct packet rec_packet;

    packet_rec(&rec_packet, peer_dest->socket_fd);

    /* if name of the file to send did not exist on the client's computer
     * file transfer is aborted */
    if (rec_packet.header.type == FILENAME) {
        return 0;
    }

    /* receiving data of the file from client and saving it into a new file on our computer */
    /* creating directory where the received file will be stored */
    if (mkdir("inbox", S_IRWXU | S_IRWXG | S_IRWXO) && errno != EEXIST) {
        perror("mkdir:");
    }

    char filename_dest[FILENAME_LEN];

    /* creating the path of the received file */
    snprintf(filename_dest, FILENAME_LEN, "inbox/%s", file_to_receive);

    /* opening the file */
    int fd_new_file = open(filename_dest, O_RDWR | O_CREAT | O_APPEND | O_TRUNC, S_IRWXU);

    if (fd_new_file == -1) {
        perror("creating file");
        exit(EXIT_FAILURE);
    }

    long size = strtol(rec_packet.payload, NULL, 10);
    ssize_t ret = 0, offset = 0;

    /* while we did not receive all the data of the file */
    while (offset != size) {

        packet_rec(&rec_packet, peer_dest->socket_fd);

        /* writing received data in a new file */
        if (-1 == (ret = write(fd_new_file, rec_packet.payload, rec_packet.header.len_payload))) {
            perror("Writing in new file");
            exit(EXIT_FAILURE);
        }

        offset += ret;

        struct packet res_packet = packet_init(peer_dest->nickname, FILE_ACK, "", "", 0);
        packet_send(&res_packet, peer_dest->socket_fd);

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

    if (close(fd_new_file) == -1) {
        perror("Erreur lors de la fermeture du fichier");
        exit(EXIT_FAILURE);
    }

    printf("File received successfully and saved in %s\n", filename_dest);

    return 1;
}

struct peer peer_init_peer_src(char *hostname, char *port, char *nickname_user) {
    struct peer peer_src;

    memset(&peer_src, 0, sizeof(struct peer));

    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, port, &hints, &result) != 0) {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        peer_src.socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (peer_src.socket_fd == -1) {
            continue;
        }

        if (connect(peer_src.socket_fd, rp->ai_addr, rp->ai_addrlen) != -1) {
            /* getting ip address and port of connection */
            struct sockaddr_in *sockaddr_in_ptr = (struct sockaddr_in *) rp->ai_addr;
            socklen_t len = sizeof(struct sockaddr_in);
            getsockname(peer_src.socket_fd, (struct sockaddr *) sockaddr_in_ptr, &len);
            peer_src.port_num = ntohs(sockaddr_in_ptr->sin_port);
            inet_ntop(AF_INET, &((struct sockaddr_in *) &sockaddr_in_ptr)->sin_addr, peer_src.ip_addr, INET_ADDRSTRLEN);
            snprintf(peer_src.nickname, NICK_LEN, "%s", nickname_user);
            printf("You (%s:%hu) are now connected to %s:%s\n", peer_src.ip_addr, peer_src.port_num, hostname, port);

            break;
        }

        close(peer_src.socket_fd);
    }

    if (rp == NULL) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

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
    int file_fd = open(file_to_send, O_RDONLY);

    if (file_fd == -1) {

        if (errno == ENOENT) {
            return 0;
        }

        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    /* getting the size of the file to send */
    long size = sb.st_size;

    char payload[MSG_LEN];
    memset(payload, 0, MSG_LEN);
    snprintf(payload, MSG_LEN, "%lu", size);

    /* sending the size of the file to be sent */
    struct packet packet = packet_init(peer_src->nickname, FILE_SEND, "", payload, strlen(payload));
    packet_send(&packet, peer_src->socket_fd);


    long offset = 0;
    /* while file not totally sent */
    /* sending file by frames of 1024 Bytes */
    while (offset != size) {
        /* putting frame of 1023 Bytes in payload */
        memset(payload, 0, MSG_LEN);
        ssize_t ret = read(file_fd, payload, MSG_LEN);

        if (-1 == ret) {
            perror("Reading from client socket");
            exit(EXIT_FAILURE);
        }

        if (0 == ret) {
            printf("Should close connection, read 0 bytes\n");
            close(file_fd);
            break;
        }

        packet_set(&packet, peer_src->nickname, FILE_SEND, "", payload, ret);
        packet_send(&packet, peer_src->socket_fd);

        /* waiting data to be read by dest user in the socket file*/
        packet_rec(&packet, peer_src->socket_fd);

        if (packet.header.type != FILE_ACK) {
            printf("Problem in reception\n");
            return 0;
        }

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

    if (close(file_fd) == -1) {
        perror("Erreur lors de la fermeture du fichier");
        exit(EXIT_FAILURE);
    }

    printf("File sent successfully and received successfully\n");

    return 1;
}