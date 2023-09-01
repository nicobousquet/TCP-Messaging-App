#include "../../include/packet/header.h"
#include <string.h>
#include <stdlib.h>

struct header *header_init() {
    struct header *header = malloc(sizeof(struct header));
    memset(header, 0, sizeof(struct header));
    return header;
}

void header_free(struct header *header) {
    free(header);
}

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
