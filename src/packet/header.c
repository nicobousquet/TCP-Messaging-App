#include "../../include/packet/header.h"
#include <string.h>

void header_set(struct header *header, unsigned long len_payload, char *from, enum messageType type, char *infos) {
    if (from != NULL) {
        memset(header->from, 0, 2 * NICK_LEN);
        strncpy(header->from, from, 2 * NICK_LEN - 1);
    }

    header->type = type;

    if (infos != NULL) {
        memset(header->infos, 0, INFOS_LEN);
        strncpy(header->infos, infos, INFOS_LEN - 1);
    }

    header->len_payload = len_payload;
}

struct header header_init(unsigned long len_payload, char *from, enum messageType type, char *infos) {
    struct header header;
    header_set(&header, len_payload, from, type, infos);

    return header;
}
