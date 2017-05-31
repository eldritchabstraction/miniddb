#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <ev.h>

#define LISTEN_PORT 3033
#define BUFFER_SIZE 1024

int total_clients = 0;  // Total number of connected clients

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

int main()
{
    struct ev_loop *loop = ev_default_loop(0);
    int listen_socket_fd;
    struct sockaddr_in listen_socket;
    struct ev_io w_accept;

    // Create listen socket
    if( (listen_socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
    {
        perror("socket error");
        return -1;
    }

    memset(&listen_socket, 0, sizeof(listen_socket));
    listen_socket.sin_family = AF_INET;
    listen_socket.sin_port = htons(LISTEN_PORT);
    listen_socket.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to address
    if (bind(listen_socket_fd, (struct sockaddr*) &listen_socket, sizeof(listen_socket)) != 0)
    {
        perror("bind error");
        return -1;
    }

    // Start listener on the socket
    if (listen(listen_socket_fd, 32) < 0)
    {
        perror("listen error");
        return -1;
    }

    // Initialize and start a watcher to accepts client requests
    ev_io_init(&w_accept, accept_cb, listen_socket_fd, EV_READ);
    ev_io_start(loop, &w_accept);

    // Start infinite loop
    while (1)
    {
        ev_loop(loop, 0);
    }

    return 0;
}

/* Accept client requests */
void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_socket_fd;
    struct ev_io *w_client = (struct ev_io*) malloc (sizeof(struct ev_io));

    if(EV_ERROR & revents)
    {
        perror("got invalid event");
        return;
    }

    // Accept client request
    client_socket_fd = accept(watcher->fd, (struct sockaddr *)&client_addr, &client_len);

    if (client_socket_fd < 0)
    {
        perror("accept error");
        return;
    }

    total_clients++; // Increment total_clients count
    printf("Successfully connected with client.\n");
    printf("%d client(s) connected.\n", total_clients);

    // Initialize and start watcher to rc client requests
    ev_io_init(w_client, read_cb, client_socket_fd, EV_READ);
    ev_io_start(loop, w_client);
}

/* rc client message */
void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents){
    char buffer[BUFFER_SIZE];
    int rc;

    if(EV_ERROR & revents)
    {
        perror("got invalid event");
        return;
    }

    // Receive message from client socket
    rc = recv(watcher->fd, buffer, BUFFER_SIZE, 0);

    if(rc < 0)
    {
        perror("read error");
        return;
    }
    else if(rc == 0)
    {
        // Stop and free watcher if client socket is closing
        ev_io_stop(loop,watcher);
        free(watcher);
        total_clients --; // Decrement total_clients count
        printf("%d client(s) connected.\n", total_clients);
        return;
    }
    else
    {
        printf("message:%s\n",buffer);
    }
}
