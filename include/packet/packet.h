#ifndef PACKET_H
#define PACKET_H

#include "header.h"
#include <sys/types.h>

struct packet {
    struct header header;
    char payload[MSG_LEN];
};

struct packet packet_init(char *from, enum messageType type, char *infos, char *payload, unsigned long payload_length);

void packet_set(struct packet *packet, char *from, enum messageType type, char *infos, char *payload, unsigned long payload_length);

void packet_send(struct packet *packet, int fd_dest);

ssize_t packet_rec(struct packet *packet, int socket_fd);

#endif //PACKET_H
