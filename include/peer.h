#ifndef TCP_MESSAGING_APP_PEER_H
#define TCP_MESSAGING_APP_PEER_H

#include "packet.h"
#include <sys/types.h>

struct peer {
    int socket_fd;
    u_short port_num;
    char ip_addr[16];
    char nickname[NICK_LEN];
    char buffer[MSG_LEN];
    struct packet packet;
};

struct peer *peer_init_peer_dest(char *listening_port, char *nickname_user);

int peer_receive_file(struct peer *peer_dest, char *file_to_receive);

struct peer *peer_init_peer_src(char *hostname, char *port);

int peer_send_file(struct peer *peer_src, char *file_to_send);

#endif //PEER_H
