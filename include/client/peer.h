#ifndef TCP_MESSAGING_APP_PEER_H
#define TCP_MESSAGING_APP_PEER_H

#include "../packet/packet.h"
#include <sys/types.h>
#include <arpa/inet.h>

struct peer {
    int socket_fd;
    u_short port_num;
    char ip_addr[INET_ADDRSTRLEN];
    char nickname[NICK_LEN];
};

struct peer peer_init_peer_receiver(char *listening_port, char *nickname_user);

int peer_receive_file(struct peer *peer_receiver, char *file_to_receive);

struct peer peer_init_peer_sender(char *peer_receiver_ip_addr, char *peer_receiver_port, char *nickname_user);

int peer_send_file(struct peer *peer_sender, char *file_to_send);

#endif //PEER_H
