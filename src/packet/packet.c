#include "../../include/packet/packet.h"
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* copying information in header */
void packet_set(struct packet *packet, char *from, enum messageType type, char *infos, char *payload) {
    header_set(&packet->header, strlen(payload), from, type, infos);

    if (payload != NULL) {
        strcpy(packet->payload, payload);
    }
}

/* send structure and payload */
void packet_send(struct packet *packet, int fd_dest) {
    if (send(fd_dest, &packet->header, sizeof(struct header), 0) <= 0) {
        perror("packet_send");
        exit(EXIT_FAILURE);
    }

    if (packet->header.len_payload != 0 && send(fd_dest, packet->payload, packet->header.len_payload, 0) <= 0) {
        perror("packet_send");
        exit(EXIT_FAILURE);
    }
}
