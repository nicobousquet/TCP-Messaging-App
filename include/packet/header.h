#ifndef HEADER_H
#define HEADER_H

#include "../constants.h"

/* different types of messages */
enum messageType {
    DEFAULT,
    NICKNAME_NEW,
    NICKNAME_LIST,
    NICKNAME_INFOS,
    ECHO_SEND,
    UNICAST_SEND,
    BROADCAST_SEND,
    MULTICAST_CREATE,
    MULTICAST_LIST,
    MULTICAST_JOIN,
    MULTICAST_SEND,
    MULTICAST_QUIT,
    FILE_REQUEST,
    FILE_ACCEPT,
    FILE_REJECT,
    FILE_SEND,
    FILE_ACK,
    FILENAME,
    HELP
};

struct header {
    unsigned long len_payload; /* length of the payload */
    char from[2 * NICK_LEN]; /* name of the sender */
    enum messageType type; /* type of the message */
    char infos[INFOS_LEN]; /* additional infos on the message */
};

struct header *header_init();

void header_free(struct header *header);

void header_set(struct header *header, unsigned long len_payload, char *from, enum messageType type, char *infos);

#endif //HEADER_H
