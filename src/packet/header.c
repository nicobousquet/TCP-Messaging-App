#include "../../include/packet/header.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct header header_init(unsigned long len_payload, char *from, enum message_type type, char *infos) {
    struct header header;
    header_set(&header, len_payload, from, type, infos);

    return header;
}

void header_set(struct header *header, unsigned long len_payload, char *from, enum message_type type, char *infos) {
    if (from != NULL) {
        memset(header->from, 0, NICK_LEN);
        snprintf(header->from, NICK_LEN, "%s", from);
    }

    header->type = type;

    if (infos != NULL) {
        memset(header->infos, 0, INFOS_LEN);
        snprintf(header->infos, INFOS_LEN, "%s", infos);
    }

    header->len_payload = len_payload;
}

void header_send(struct header *header, int socket_fd) {
    if (send(socket_fd, header, sizeof(struct header), 0) == -1) {
        perror("header_send");
        exit(EXIT_FAILURE);
    }
}

ssize_t header_rec(struct header *header, int socket_fd) {
    memset(header, 0, sizeof(struct header));

    ssize_t ret = -1;

    if ((ret = recv(socket_fd, header, sizeof(struct header), MSG_WAITALL)) == -1) {
        perror("header_rec");
        exit(EXIT_FAILURE);
    }

    return ret;
}
