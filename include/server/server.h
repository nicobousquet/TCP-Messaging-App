#ifndef SERVER_H
#define SERVER_H

#include "../packet/packet.h"
#include "chatroom.h"

struct server {
    int socket_fd;
    char ip_addr[INET_ADDRSTRLEN];
    u_short port_num;
    struct chatroom *list_chatrooms[NUM_MAX_CHATROOMS];
    int num_chatrooms; /* number of chatrooms not empty */
    struct user_node *user_head;
    int num_users;
    struct user_node *current_user;
};

struct server server_init(char *port);

void server_disconnect_user(struct server *server, struct user_node *user_to_disconnect);

void server_handle_file_reject_req(struct server *server, struct packet *req_packet);

void server_handle_file_accept_res(struct server *server, struct packet *req_packet);

void server_handle_file_req(struct server *server, struct packet *req_packet);

void server_handle_multicast_send_req(struct server *server, struct packet *req_packet);

void server_handle_multicast_quit_req(struct server *server, struct packet *req_packet);

void server_handle_multicast_join_req(struct server *server, struct packet *req_packet);

void server_handle_multicast_list_req(struct server *server, struct packet *req_packet);

void server_handle_multicast_create_req(struct server *server, struct packet *req_packet);

void server_handle_unicast_send_req(struct server *server, struct packet *req_packet);

void server_handle_broadcast_send_req(struct server *server, struct packet *req_packet);

void server_handle_nickname_infos_req(struct server *server, struct packet *req_packet);

void server_handle_nickname_list_req(struct server *server, struct packet *req_packet);

void server_handle_nickname_new_req(struct server *server, struct packet *req_packet);

#endif //SERVER_H
