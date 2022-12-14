#include "../include/common.h"
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>

/* creating a server socket */
struct socket socketAndBind(char *port) {
    struct addrinfo hints, *result, *rp;
    struct socket serverSocket;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, port, &hints, &result) != 0) {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        serverSocket.fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (serverSocket.fd == -1) {
            continue;
        }
        if (bind(serverSocket.fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            /* getting ip address  and port*/
            struct sockaddr_in my_addr;
            socklen_t len = sizeof(my_addr);
            getsockname(serverSocket.fd, (struct sockaddr *) &my_addr, &len);
            inet_ntop(AF_INET, &my_addr.sin_addr, serverSocket.ipAddr, 16);
            serverSocket.port = ntohs(my_addr.sin_port);
            freeaddrinfo(result);
            return serverSocket;
        }
        close(serverSocket.fd);
    }
    perror("bind()");
    exit(EXIT_FAILURE);
}

/* send structure and payload */
void sendMsg(int clientFd, struct message *msgStruct, char *payload) {
    if (send(clientFd, msgStruct, sizeof(struct message), 0) <= 0) {
        perror("SendMsg");
        exit(EXIT_FAILURE);
    }

    if (msgStruct->payloadLen != 0 && send(clientFd, payload, msgStruct->payloadLen, 0) <= 0) {
        perror("SendMsg");
        exit(EXIT_FAILURE);
    }
}
