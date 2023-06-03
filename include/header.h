#ifndef HEADER_H
#define HEADER_H

#include "constants.h"

/* different types of messages */
enum message_type {
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
    HELP,
    QUIT
};

struct Header {
    unsigned long len_payload; /* length of the payload */
    char from[2 * NICK_LEN]; /* name of the sender */
    enum message_type type; /* type of the message */
    char infos[INFOS_LEN]; /* additional infos on the message */
};

void header_set(struct Header *header, unsigned long len_payload, char *from, enum message_type type, char *infos);

#endif //HEADER_H
