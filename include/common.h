#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>

#define MSG_LEN 1024
#define NICK_LEN 128
#define INFOS_LEN 128

/* different types of messages */
enum msgType {
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

struct socket {
    int fd;
    char ipAddr[16];
    u_short port;
};

/* structure of a message */
struct header {
    unsigned long payloadLen; /* length of the payload */
    char nickSender[2 * NICK_LEN]; /* name of the sender */
    enum msgType type; /* type of the message */
    char infos[INFOS_LEN]; /* additional infos on the message */
};

struct packet {
    struct header header;
    char payload[MSG_LEN];
};

struct socket socketAndBind(char *port);

void sendPacket(int clientFd, struct packet *packet);

#endif //COMMON_H
