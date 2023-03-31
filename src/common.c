#include "../include/common.h"
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>

/* copying information in header */
void setPacket(struct packet *packet, char *nickSender, enum msgType type, char *infos, char *payload) {
    if (nickSender != NULL) {
        strcpy(packet->header.nickSender, nickSender);
    }
    packet->header.type = type;
    if (infos != NULL) {
        strcpy(packet->header.infos, infos);
    }
    if (payload != NULL) {
        strcpy(packet->payload, payload);
    }
    packet->header.payloadLen = strlen(packet->payload);
}

/* send structure and payload */
void sendPacket(int clientFd, struct packet *packet) {
    if (send(clientFd, &packet->header, sizeof(struct header), 0) <= 0) {
        perror("sendPacket");
        exit(EXIT_FAILURE);
    }

    if (packet->header.payloadLen != 0 && send(clientFd, packet->payload, packet->header.payloadLen, 0) <= 0) {
        perror("sendPacket");
        exit(EXIT_FAILURE);
    }
}
