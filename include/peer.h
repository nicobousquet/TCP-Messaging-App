#ifndef TCP_MESSAGING_APP_PEER_H
#define TCP_MESSAGING_APP_PEER_H

#include "packet.h"
#include <sys/types.h>

struct Peer {
    int fd_socket;
    u_short port_num;
    char ip_addr[16];
    char nickname[NICK_LEN];
    char buffer[MSG_LEN];
    struct Packet packet;
};

struct Peer *peer_init_peer_dest(char *listening_port, char *nickname_user);

int peer_receive_file(struct Peer *server_p2p, char *file_to_receive);

struct Peer *peer_init_peer_src(char *hostname, char *port);

int peer_send_file(struct Peer *peer_src, char *file_to_send);

#endif //PEER_H
