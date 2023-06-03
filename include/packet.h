#ifndef PACKET_H
#define PACKET_H

#include "../include/header.h"

struct Packet {
    struct Header header;
    char payload[MSG_LEN];
};

void packet_set(struct Packet *packet, char *from, enum message_type type, char *infos, char *payload);

void packet_send(struct Packet *packet, int fd_dest);

#endif //PACKET_H
