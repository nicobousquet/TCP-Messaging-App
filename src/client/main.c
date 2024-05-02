#include "../../include/client/client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void usage() {
    printf("Usage: ./client hostname port_number\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        usage();
    }

    struct client client = client_init(argv[1], argv[2]);

    /* running client */
    /* Declare array of struct pollfd */
    struct pollfd pollfds[NUM_FDS];

    /* Init first slots with listening socket  to receive data */
    /* from server */
    pollfds[0].fd = client.socket_fd;
    pollfds[0].events = POLLIN;
    pollfds[0].revents = 0;
    /* from keyboard */
    pollfds[1].fd = STDIN_FILENO;
    pollfds[1].events = POLLIN;
    pollfds[1].revents = 0;

    /* client loop */
    while (1) {
        fflush(stdout);

        if (-1 == poll(pollfds, NUM_FDS, -1)) {
            perror("Poll");
        }

        for (int i = 0; i < NUM_FDS; i++) {

            /* if data comes from the keyboard */
            if (pollfds[i].fd == STDIN_FILENO && pollfds[i].revents & POLLIN) {
                /* putting data into buffer */
                char buffer[MSG_LEN] = {0};

                int n = 0;
                char c;

                while ((c = (char) getchar()) != '\n' && n != MSG_LEN) {
                    buffer[n++] = c;
                }

                /* move to the beginning of previous line */
                printf("\033[1A\33[2K\r");
                fflush(stdout);

                /* getting command entered by user */
                char *first_word = strtok(buffer, " ");

                if (first_word != NULL) {
                    if (strcmp(first_word, "/nick") == 0) {
                        client_send_nickname_new_req(&client, strtok(NULL, " "));

                    } else if (strcmp(first_word, "/help") == 0) {
                        client_help();

                    } else if (strcmp(first_word, "/who") == 0) {
                        client_send_nickname_list_req(&client);

                    } else if (strcmp(first_word, "/whois") == 0) {
                        client_send_nickname_infos_req(&client, strtok(NULL, ""));

                    } else if (strcmp(first_word, "/msgall") == 0) {
                        client_send_broadcast_send_req(&client, strtok(NULL, ""));

                    } else if (strcmp(first_word, "/msg") == 0) {
                        char *nickname_dest = strtok(NULL, " ");
                        char *payload = strtok(NULL, "");

                        client_send_unicast_send_req(&client, nickname_dest, payload);

                    } else if (strcmp(first_word, "/create") == 0) {
                        client_send_multicast_create_req(&client, strtok(NULL, ""));

                    } else if (strcmp(first_word, "/channel_list") == 0) {
                        client_send_multicast_list_req(&client);

                    } else if (strcmp(first_word, "/join") == 0) {
                        client_send_multicast_join_req(&client, strtok(NULL, ""));

                    } else if (strcmp(first_word, "/quit") == 0) {
                        char *name_channel = strtok(NULL, "");
                        if (name_channel != NULL) {
                            client_send_multicast_quit_req(&client, name_channel);
                        } else {
                            client_disconnect_from_server(pollfds);
                            exit(EXIT_SUCCESS);
                        }

                    } else if (strcmp(first_word, "/send") == 0) {
                        char *nickname_dest = strtok(NULL, " ");
                        char *filename = strtok(NULL, "");

                        client_send_file_req(&client, nickname_dest, filename);

                    } else {
                        client_send_multicast_send_req(&client, strtok(NULL, ""), first_word);
                    }
                }

                pollfds[i].revents = 0;
            }
                /* if data comes from the server */
            else if (pollfds[i].fd != STDIN_FILENO && pollfds[i].revents & POLLIN) {
                /* Receiving structure */
                struct packet res_packet;

                if (packet_rec(&res_packet, client.socket_fd) <= 0) {
                    printf("Connection lost with the server\n");
                    client_disconnect_from_server(pollfds);
                    exit(EXIT_FAILURE);
                }

                switch (res_packet.header.type) {

                        /* changing nickname */
                    case NICKNAME_NEW:
                        client_handle_nickname_new_res(&client, &res_packet);
                        printf("[%s]: %s\n", res_packet.header.from, res_packet.payload);
                        pollfds[i].revents = 0;
                        break;

                        /* if receiving a file request */
                    case FILE_REQUEST:
                        client_handle_file_request_res(&client, &res_packet);
                        pollfds[i].revents = 0;
                        break;

                        /* if receiving a file accept */
                    case FILE_ACCEPT:
                        client_handle_file_accept_res(&client, &res_packet);
                        pollfds[i].revents = 0;
                        break;

                    default:
                        printf("[%s]: %s\n", res_packet.header.from, res_packet.payload);
                        pollfds[i].revents = 0;
                        break;
                }
            }
        }
    }
}
