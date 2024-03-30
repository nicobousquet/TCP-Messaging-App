#include "../../include/packet/packet.h"
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* copying information in header */
void packet_set(struct packet *packet, char *from, enum messageType type, char *infos, char *payload) {
    header_set(&packet->header, strnlen(payload, MSG_LEN - 1), from, type, infos);

    if (payload != NULL) {
        memset(packet->payload, 0, MSG_LEN);
        strncpy(packet->payload, payload, MSG_LEN - 1);
    }
}

struct packet packet_init(char *from, enum messageType type, char *infos, char *payload) {
    struct packet packet;
    memset(&packet, 0, sizeof(struct packet));
    packet_set(&packet, from, type, infos, payload);

    return packet;
}

/* send structure and payload */
void packet_send(struct packet *packet, int fd_dest) {
    if (send(fd_dest, &packet->header, sizeof(struct header), 0) <= 0) {
        perror("packet_send");
        exit(EXIT_FAILURE);
    }

    if (packet->header.len_payload != 0) {

        if (send(fd_dest, packet->payload, packet->header.len_payload, 0) <= 0) {
            perror("packet_send");
            exit(EXIT_FAILURE);
        }
    }
}

ssize_t packet_rec(struct packet *packet, int socket_fd) {

    memset(&packet, 0, sizeof(struct packet));

    ssize_t ret = -1;

    if ((ret = recv(socket_fd, &packet->header, sizeof(struct header), MSG_WAITALL)) <= 0) {
        return ret;
    }

    if (packet->header.len_payload != 0) {
        int ret2 = -1;

        if ((ret2 = recv(socket_fd, packet->payload, packet->header.len_payload, MSG_WAITALL)) <= 0) {
            perror("recv");
            exit(EXIT_FAILURE);
        }

        ret = ret + ret2;
    }

    return ret;
}
