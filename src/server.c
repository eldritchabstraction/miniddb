#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for read
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <poll.h>

#include <errno.h>


#include "server.h"

#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(fmt, args...) printf("DEBUG: " fmt, ##args)
#define DEBUG_PRINT_CLEAN(fmt, args...) printf(fmt, ##args)
#else
#define DEBUG_PRINT(x) do {} while(0)
#define DEBUG_PRINT_CLEAN do{} while(0)
#endif

#define ERROR_PRINT(fmt, args...) printf("ERROR: " fmt, ##args)


void server_start(uint16_t bind_port)
{
    int listen_sockfd = 0, connection_sockfd = 0, client_address_size = 0;
    struct sockaddr_in server_address, client_address;
    char buffer[256];

    // Get a socket
    if ((listen_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        ERROR_PRINT("opening socket: %s\n", strerror(errno));
        exit(1);
    }

    // Allow socket descriptor to be reusable
    int on = 1;
    if (setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)(&on), sizeof(on)) < 0)
    {
        ERROR_PRINT("setsockopt failed: %s\n", strerror(errno));
        close(listen_sockfd);
        exit(1);
    }

    // Set socket to be nonblocking, connection sockets inherit from listening
    // socket

    /* TODO: uncomment me after poll loop is implemented
    if (ioctl(listen_sockfd, FIONBIO, (char*)(&on)) < 0)
    {
        ERROR_PRINT("ioctl failed\n");
        close(listen_sockfd);
        exit(1);
    }
    */

    // Initialize socket structure
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(bind_port);

    // Bind to socket
    if (bind(listen_sockfd, (struct sockaddr *)&server_address,
        sizeof(server_address)) < 0)
    {
        ERROR_PRINT("on bind: %s\n", strerror(errno));
        close(listen_sockfd);
        exit(1);
    }

    // Listen on socket
    if (listen(listen_sockfd, 32) < 0)
    {
        ERROR_PRINT("listen failed: %s\n", strerror(errno));
        close(listen_sockfd);
        exit(1);
    }

    // Accept connection from client

    client_address_size = sizeof(client_address);
    if ((connection_sockfd = accept(listen_sockfd, (struct sockaddr *)&client_address,
        &client_address_size)) < 0)
    {
        ERROR_PRINT("on accept: %s\n", strerror(errno));
        close(listen_sockfd);
        exit(1);
    }

    // If connection is established, then start reading

    memset(buffer, 0, sizeof(buffer));

    if (read(connection_sockfd, buffer, 255) < 0)
    {
        ERROR_PRINT("on read: %s\n", strerror(errno));
        close(listen_sockfd);
        close(connection_sockfd);
    }

    printf("Here is the message %s\n", buffer);

    close(connection_sockfd);
}
