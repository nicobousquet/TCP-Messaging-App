#include "../../include/server/server.h"
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

char *msg_type_str[] = {
        "DEFAULT",
        "NICKNAME_NEW",
        "NICKNAME_LIST",
        "NICKNAME_INFOS",
        "ECHO_SEND",
        "UNICAST_SEND",
        "BROADCAST_SEND",
        "MULTICAST_CREATE",
        "MULTICAST_LIST",
        "MULTICAST_JOIN",
        "MULTICAST_SEND",
        "MULTICAST_QUIT",
        "FILE_REQUEST",
        "FILE_ACCEPT",
        "FILE_REJECT",
        "FILE_SEND",
        "FILE_ACK",
        "FILENAME"
};

static void usage() {
    printf("Usage: ./server port_number\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage();
    }

    struct server server = server_init(argv[1]);

    /* Declare array of struct pollfd */
    struct pollfd pollfds[NUM_MAX_USERS + 1];

    /* Init first slot with listening socket */
    pollfds[0].fd = server.socket_fd;
    pollfds[0].events = POLLIN;
    pollfds[0].revents = 0;

    /* Init remaining slot to default values */
    for (int i = 1; i < NUM_MAX_USERS + 1; i++) {
        pollfds[i].fd = -1;
        pollfds[i].events = 0;
        pollfds[i].revents = 0;
    }

    /* server loop */
    while (1) {
        /* Block until new activity detected on existing sockets */
        if (poll(pollfds, NUM_MAX_USERS + 1, -1) == -1) {
            perror("Poll");
        }

        /* Iterate over the array of monitored struct pollfd */
        for (int i = 0; i < NUM_MAX_USERS + 1; i++) {
            /* If listening socket is active => accept new incoming connection */
            if (pollfds[i].fd == server.socket_fd && pollfds[i].revents & POLLIN) {
                /* accept new connection and retrieve new socket file descriptor */
                struct sockaddr_in clientAddr;
                socklen_t len = sizeof(clientAddr);
                int socket_fd = -1;

                if (-1 == (socket_fd = accept(server.socket_fd, (struct sockaddr *) &clientAddr, &len))) {
                    perror("Accept");
                }

                if (server.num_users < NUM_MAX_USERS) {

                    /* adding a new user */
                    time_t ltime;
                    time(&ltime);

                    struct user_node *user = user_node_init(socket_fd, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), "", asctime(localtime(&ltime)));
                    printf("Client (%s:%hu) connected on socket %i.\n", user->ip_addr, user->port_num, user->socket_fd);
                    server_add_user_node(&server, user);

                    /* store new file descriptor in available slot in the array of struct pollfd set .events to POLLIN */
                    for (int j = 1; j < NUM_MAX_USERS + 1; j++) {
                        if (pollfds[j].fd == -1) {
                            pollfds[j].fd = socket_fd;
                            pollfds[j].events = POLLIN;
                            break;
                        }
                    }

                    char payload[] = "Please, login with /nick <your nickname>";
                    struct packet res_packet = packet_init("SERVER", DEFAULT, "", payload, strlen(payload));
                    packet_send(&res_packet, socket_fd);

                } else {
                    char payload[] = "Server at capacity! Cannot accept more connections :(";
                    struct packet res_packet = packet_init("SERVER", DEFAULT, "", payload, strlen(payload));
                    packet_send(&res_packet, socket_fd);

                    close(socket_fd);
                }

                /* Set .revents of listening socket back to default */
                pollfds[i].revents = 0;
            } else if (pollfds[i].fd != server.socket_fd && pollfds[i].revents & POLLHUP) {
                /* getting the user which is doing the request */
                struct user_node *current_user = NULL;

                for (current_user = server.user_head; current_user != NULL; current_user = current_user->next_in_server) {
                    if (current_user->socket_fd == pollfds[i].fd) {
                        break;
                    }
                }

                /* If a socket previously created to communicate with a client detects a disconnection from the client side */
                server_disconnect_user(&server, current_user);

                /* Close socket and set struct pollfd back to default */
                close(pollfds[i].fd);
                pollfds[i].fd = -1;
                pollfds[i].events = 0;
                pollfds[i].revents = 0;
            } else if (pollfds[i].fd != server.socket_fd && pollfds[i].revents & POLLIN) {
                /* If a socket different from the listening socket is active */
                /* getting the user which is doing the request */
                struct user_node *current_user = NULL;

                for (current_user = server.user_head; current_user != NULL; current_user = current_user->next_in_server) {
                    if (current_user->socket_fd == pollfds[i].fd) {
                        break;
                    }
                }

                server.current_user = current_user;

                struct packet req_packet = {0};

                if (packet_rec(&req_packet, server.current_user->socket_fd) <= 0) {
                    server_disconnect_user(&server, server.current_user);
                    close(pollfds[i].fd);
                    pollfds[i].fd = -1;
                    pollfds[i].events = 0;
                    pollfds[i].revents = 0;
                    break;
                }

                printf("len_payload: %ld / from: %s / type: %s / infos: %s\n", req_packet.header.len_payload, req_packet.header.from, msg_type_str[req_packet.header.type], req_packet.header.infos);
                printf("payload: %s\n", req_packet.payload);

                if (!server.current_user->is_logged_in && req_packet.header.type != NICKNAME_NEW) {
                    char payload[] = "Please, login with /nick <your nickname>";
                    struct packet res_packet = packet_init("SERVER", DEFAULT, "", payload, strlen(payload));
                    packet_send(&res_packet, server.current_user->socket_fd);

                    break;
                }

                switch (req_packet.header.type) {
                    /* if user wants to change/create nickname */
                    case NICKNAME_NEW:
                        server_handle_nickname_new_req(&server, &req_packet);
                        break;

                        /* if user wants to see the list of other connected users */
                    case NICKNAME_LIST:
                        server_handle_nickname_list_req(&server, &req_packet);
                        break;

                        /* if user wants to know the ip address, remote port number and connection date of another user */
                    case NICKNAME_INFOS:
                        server_handle_nickname_infos_req(&server, &req_packet);
                        break;

                        /* if user wants to send a message to all the users */
                    case BROADCAST_SEND:
                        server_handle_broadcast_send_req(&server, &req_packet);
                        break;

                        /* if user wants to send a message to another specific user */
                    case UNICAST_SEND:
                        server_handle_unicast_send_req(&server, &req_packet);
                        break;

                        /* if user wants to create a chatroom_node */
                    case MULTICAST_CREATE:
                        server_handle_multicast_create_req(&server, &req_packet);
                        break;

                        /* if user wants to have the list of all the chatrooms created */
                    case MULTICAST_LIST:
                        server_handle_multicast_list_req(&server, &req_packet);
                        break;

                        /* if user wants to join a chatroom_node */
                    case MULTICAST_JOIN:
                        server_handle_multicast_join_req(&server, &req_packet);
                        break;

                        /* if user wants to quit a chatroom_node */
                    case MULTICAST_QUIT:
                        server_handle_multicast_quit_req(&server, &req_packet);
                        break;

                        /* if user wants to send a message in the chatroom_node */
                    case MULTICAST_SEND:
                        server_handle_multicast_send_req(&server, &req_packet);
                        break;

                        /* if user wants to send a file to another user */
                    case FILE_REQUEST:
                        server_handle_file_req(&server, &req_packet);
                        break;

                        /* if user accept file transfer */
                    case FILE_ACCEPT:
                        server_handle_file_accept_res(&server, &req_packet);
                        break;

                        /* if user rejects file transfer */
                    case FILE_REJECT:
                        server_handle_file_reject_req(&server, &req_packet);
                        break;

                    default:
                        break;
                }

                /* set back .revents to 0 */
                pollfds[i].revents = 0;
            }
        }
    }
}