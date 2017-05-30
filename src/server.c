#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for read
#include <sys/socket.h>
#include <arpa/inet.h>

#include "server.h"

#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(fmt, args...) printf("DEBUG: " fmt, ##args)
#define DEBUG_PRINT_CLEAN(fmt, args...) printf(fmt, ##args)
#else
#define DEBUG_PRINT(x) do {} while(0)
#define DEBUG_PRINT_CLEAN do{} while(0)
#endif

#define ERROR_PRINT(fmt, args...) { printf("ERROR: " fmt, ##args); exit(1); }


void server_start(uint16_t bind_port)
{
    int sockfd = 0, connection_sockfd = 0, client_address_size = 0;
    struct sockaddr_in server_address, client_address;
    char buffer[256];

    // Get a socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        ERROR_PRINT("opening socket\n");

    // Initialize socket structure
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(bind_port);

    // Bind to socket
    if (bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        ERROR_PRINT("on bind\n");

    // Listen on socket

    listen(sockfd, 5);

    // Accept connection from client

    client_address_size = sizeof(client_address);
    if ((connection_sockfd = accept(sockfd, (struct sockaddr *)&client_address,
        &client_address_size)) < 0)
        ERROR_PRINT("on accept\n");

    // If connection is established, then start reading

    memset(buffer, 0, sizeof(buffer));

    if (read(connection_sockfd, buffer, 255) < 0)
        ERROR_PRINT("on read\n");

    printf("Here is the message %s\n", buffer);

    close(connection_sockfd);
}
