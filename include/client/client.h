#ifndef CLIENT_H
#define CLIENT_H

#define NUM_FDS 2

#include "../packet/packet.h"
#include <sys/types.h>
#include <poll.h>
#include <arpa/inet.h>

struct client {
    int socket_fd;
    char ip_addr[INET_ADDRSTRLEN];
    u_short port_num;
    char nickname[NICK_LEN];
    char file_to_send[NICK_LEN];
    char file_to_receive[NICK_LEN];
};

void client_send_nickname_new_req(struct client *client, char *new_nickname);

void client_help();

void client_send_nickname_list_req(struct client *client);

void client_send_nickname_infos_req(struct client *client, char *nickname);

void client_send_broadcast_send_req(struct client *client, char *payload);

void client_send_unicast_send_req(struct client *client, char *nickname_dest, char *payload);

void client_send_multicast_create_req(struct client *client, char *name_channel);

void client_send_multicast_list_req(struct client *client);

void client_send_multicast_join_req(struct client *client, char *chatroom);

void client_send_multicast_quit_req(struct client *client, char *name_channel);

void client_send_file_req(struct client *client, char *nickname_dest, char *filename);

void client_send_multicast_send_req(struct client *client, char *sentence, char *first_word);

void client_handle_nickname_new_res(struct client *client, struct packet *res_packet);

void client_handle_file_request_res(struct client *client, struct packet *req_packet);

void client_handle_file_accept_res(struct client *client, struct packet *res_packet);

void client_disconnect_from_server(struct pollfd *pollfds);

struct client *client_init(char *hostname, char *port);

void client_free(struct client *client);

#endif //CLIENT_H
