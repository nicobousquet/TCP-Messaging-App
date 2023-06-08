#include "../include/header.h"
#include <string.h>

void header_set(struct header *header, unsigned long len_payload, char *from, enum messageType type, char *infos) {
    if (from != NULL) {
        strcpy(header->from, from);
    }

    header->type = type;

    if (infos != NULL) {
        strcpy(header->infos, infos);
    }

    header->len_payload = len_payload;
}
