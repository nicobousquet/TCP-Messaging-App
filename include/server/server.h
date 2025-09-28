#ifndef SERVER_H
#define SERVER_H

#include "../packet/packet.h"
#include "chatroom_node.h"

struct server {
    int socket_fd;
    char ip_addr[INET_ADDRSTRLEN];
    u_short port_num;
    struct chatroom_node *chatroom_head;
    int num_chatrooms;
    struct user_node *user_head;
    int num_users;
};

struct server server_init(char *port);

void server_disconnect_user(struct server *server, struct user_node *user_to_disconnect);

void server_add_chatroom_node(struct server *server, struct chatroom_node *to_add);

void server_remove_chatroom_node(struct server *server, struct chatroom_node *to_remove);

void server_add_user_node(struct server *server, struct user_node *to_add);

void server_remove_user_node(struct server *server, struct user_node *to_remove);

struct chatroom_node *server_get_chatroom_by_user(struct chatroom_node *head, struct user_node *user);

struct chatroom_node *server_get_chatroom_by_name(struct chatroom_node *head, char *chatroom_name);

struct user_node *server_get_user_by_name(struct server *server, char *nickname_user);

void server_handle_file_reject_req(struct server *server, struct user_node *current_user, struct packet *req_packet);

void server_handle_file_accept_res(struct server *server, struct packet *req_packet);

void server_handle_file_req(struct server *server, struct user_node *current_user, struct packet *req_packet);

void server_handle_multicast_send_req(struct server *server, struct user_node *current_user, struct packet *req_packet);

void server_handle_multicast_quit_req(struct server *server, struct user_node *current_user, struct packet *req_packet);

void server_handle_multicast_join_req(struct server *server, struct user_node *current_user, struct packet *req_packet);

void server_handle_multicast_list_req(struct server *server, struct user_node *current_user, struct packet *req_packet);

void server_handle_multicast_create_req(struct server *server, struct user_node *current_user, struct packet *req_packet);

void server_handle_unicast_send_req(struct server *server, struct user_node *current_user, struct packet *req_packet);

void server_handle_broadcast_send_req(struct server *server, struct user_node *current_user, struct packet *req_packet);

void server_handle_nickname_infos_req(struct server *server, struct user_node *current_user, struct packet *req_packet);

void server_handle_nickname_list_req(struct server *server, struct user_node *current_user, struct packet *req_packet);

void server_handle_nickname_new_req(struct server *server, struct user_node *current_user, struct packet *req_packet);

#endif //SERVER_H
