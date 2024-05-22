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

struct peer peer_init_peer_receiver(char *listening_port, char *nickname_user) {
    struct peer peer_receiver = {0};

    struct addrinfo hints, *res, *p;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;

    if (getaddrinfo(NULL, listening_port, &hints, &res) != 0) {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }

    for (p = res; p != NULL; p = p->ai_next) {
        peer_receiver.socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (peer_receiver.socket_fd == -1) {
            continue;
        }

        if (bind(peer_receiver.socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(peer_receiver.socket_fd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        perror("Impossible to bind the socket to an address\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    if ((listen(peer_receiver.socket_fd, SOMAXCONN)) == -1) {
        perror("listen()\n");
        exit(EXIT_FAILURE);
    }

    /* getting ip address  and port*/
    struct sockaddr_in server_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    getsockname(peer_receiver.socket_fd, (struct sockaddr *) &server_addr, &len);
    inet_ntop(AF_INET, &server_addr.sin_addr, peer_receiver.ip_addr, INET_ADDRSTRLEN);
    peer_receiver.port_num = ntohs(server_addr.sin_port);

    printf("Listening on %s:%hu\n", peer_receiver.ip_addr, peer_receiver.port_num);

    snprintf(peer_receiver.nickname, NICK_LEN, "%s", nickname_user);

    return peer_receiver;
}

int peer_receive_file(struct peer *peer_receiver, char *file_to_receive) {
    /* receiving file */
    printf("Receiving the file...\n");

    struct packet rec_packet;

    packet_rec(&rec_packet, peer_receiver->socket_fd);

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

        packet_rec(&rec_packet, peer_receiver->socket_fd);

        /* writing received data in a new file */
        if (-1 == (ret = write(fd_new_file, rec_packet.payload, rec_packet.header.len_payload))) {
            perror("Writing in new file");
            exit(EXIT_FAILURE);
        }

        offset += ret;

        struct packet res_packet = packet_init(peer_receiver->nickname, FILE_ACK, "", "", 0);
        packet_send(&res_packet, peer_receiver->socket_fd);

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

struct peer peer_init_peer_sender(char *peer_receiver_ip_addr, char *peer_receiver_port, char *nickname_user) {
    struct peer peer_sender = {0};

    struct addrinfo hints, *res, *p;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;

    if (getaddrinfo(peer_receiver_ip_addr, peer_receiver_port, &hints, &res) == -1) {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }

    for (p = res; p != NULL; p = p->ai_next) {
        peer_sender.socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (peer_sender.socket_fd == -1) {
            continue;
        }

        if (connect(peer_sender.socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(peer_sender.socket_fd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        perror("Impossible to connect to the other peer");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    /* getting ip address and peer_receiver_port of connection */
    struct sockaddr_in peer_sender_addr;
    socklen_t len = sizeof(struct sockaddr_in);

    getsockname(peer_sender.socket_fd, (struct sockaddr *) &peer_sender_addr, &len);
    peer_sender.port_num = ntohs(peer_sender_addr.sin_port);
    inet_ntop(AF_INET, &peer_sender_addr.sin_addr, peer_sender.ip_addr, INET_ADDRSTRLEN);

    printf("You (%s:%hu) are now connected to %s:%s\n", peer_sender.ip_addr, peer_sender.port_num, peer_receiver_ip_addr, peer_receiver_port);

    snprintf(peer_sender.nickname, NICK_LEN, "%s", nickname_user);

    return peer_sender;
}

int peer_send_file(struct peer *peer_sender, char *file_to_send) {
    /* sending file */
    printf("Sending the file...\n");
    struct stat sb = {0};
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

    char payload[MSG_LEN] = {0};

    snprintf(payload, MSG_LEN, "%lu", size);

    /* sending the size of the file to be sent */
    struct packet packet = packet_init(peer_sender->nickname, FILE_SEND, "", payload, strlen(payload));
    packet_send(&packet, peer_sender->socket_fd);


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

        packet_set(&packet, peer_sender->nickname, FILE_SEND, "", payload, ret);
        packet_send(&packet, peer_sender->socket_fd);

        /* waiting data to be read by dest user in the socket file*/
        packet_rec(&packet, peer_sender->socket_fd);

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
