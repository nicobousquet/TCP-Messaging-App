#ifndef PACKET_H
#define PACKET_H

#include "header.h"

struct packet {
    struct header *header;
    char payload[MSG_LEN];
};

struct packet *packet_init(struct header *header);

void packet_free(struct packet *packet);

void packet_set(struct packet *packet, char *from, enum messageType type, char *infos, char *payload);

void packet_send(struct packet *packet, int fd_dest);

#endif //PACKET_H
