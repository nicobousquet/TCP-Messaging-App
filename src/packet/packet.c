#include "../../include/packet/packet.h"
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct packet packet_init(char *from, enum message_type type, char *infos, char *payload, unsigned long payload_length) {
    struct packet packet = {0};
    packet_set(&packet, from, type, infos, payload, payload_length);

    return packet;
}

/* copying information in header */
void packet_set(struct packet *packet, char *from, enum message_type type, char *infos, char *payload, unsigned long payload_length) {
    header_set(&packet->header, payload_length, from, type, infos);

    if (payload != NULL) {
        memset(packet->payload, 0, MSG_LEN);
        memcpy(packet->payload, payload, payload_length);
    }
}

/* send structure and payload */
void packet_send(struct packet *packet, int socket_fd) {
    header_send(&packet->header, socket_fd);

    if (packet->header.len_payload != 0) {
        if (send(socket_fd, packet->payload, packet->header.len_payload, 0) == -1) {
            perror("packet_send");
            exit(EXIT_FAILURE);
        }
    }
}

ssize_t packet_rec(struct packet *packet, int socket_fd) {
    memset(packet, 0, sizeof(struct packet));

    ssize_t ret = header_rec(&packet->header, socket_fd);

    if (packet->header.len_payload != 0) {
        ssize_t ret2 = -1;

        if ((ret2 = recv(socket_fd, packet->payload, packet->header.len_payload, MSG_WAITALL)) == -1) {
            perror("packet_rec");
            exit(EXIT_FAILURE);
        }

        ret = ret + ret2;
    }

    return ret;
}