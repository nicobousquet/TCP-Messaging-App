#ifndef HEADER_H
#define HEADER_H

#include "../constants.h"
#include <sys/socket.h>

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
    FILENAME
};

struct header {
    unsigned long len_payload; /* length of the payload */
    char from[NICK_LEN]; /* name of the sender */
    enum messageType type; /* type of the message */
    char infos[INFOS_LEN]; /* additional infos on the message */
};

struct header header_init(unsigned long len_payload, char *from, enum messageType type, char *infos);

void header_set(struct header *header, unsigned long len_payload, char *from, enum messageType type, char *infos);

void header_send(struct header *header, int socket_fd);

ssize_t header_rec(struct header *header, int socket_fd);

#endif //HEADER_H