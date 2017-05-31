#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <ev.h>
#include <unistd.h>

#define LISTEN_PORT 5555
#define BUFFER_SIZE 1024
#define STDIN_FILE_NUMBER 0


int total_clients = 0;  // Total number of connected clients

const char *g_servers[2] = { "10.0.0.2", "10.0.0.3" };
#define SERVER_COUNT 2

int g_the_variable = 0;


void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void stdin_cb (struct ev_loop *loop, ev_io *w, int revents);

int main()
{
    struct ev_loop *loop = ev_default_loop(0);
    int listen_socket_fd;
    struct sockaddr_in listen_socket;
    struct ev_io w_accept;
    struct ev_io w_stdin;

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

    // Initialize and start a watcher to accept stdio
    ev_io_init(&w_stdin, stdin_cb, STDIN_FILE_NUMBER, EV_READ);
    ev_io_start(loop, &w_stdin);

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
    char buffer[1024];
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
    inet_ntop(AF_INET, &(client_addr.sin_addr), buffer, sizeof(buffer));
    printf("Successfully connected with client %s:%d.\n", buffer, client_addr.sin_port);
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
        // printf("message:%s\n",buffer);
        // instead of printing a message, how about we set a variable?

        char *endptr;
        int temp = 0;

        if ((temp = strtol(buffer, &endptr, 10)) < 0)
        {
            perror("strtol error");
            return;
        }

        g_the_variable = temp;
        printf("The variable is currently %d\n", g_the_variable);
    }
}

int send_to_peer(const char* address, int port, const char *buffer)
{
    int send_socket_fd = -1;
    struct sockaddr_in server;

    if ((send_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error");
        return -1;
    }

    server.sin_addr.s_addr = inet_addr(address);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (connect(send_socket_fd, (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return -1;
    }

    if( send(send_socket_fd, buffer, strlen(buffer) + 1, 0) < 0)
    {
        printf("Send failed");
        return -1;
    }

    return 0;
}

void stdin_cb (struct ev_loop *loop, ev_io *w, int revents)
{
    char buffer[1024];

    memset(buffer, 0, sizeof(buffer));

    if (read(0, buffer, sizeof(buffer)) < 0)
    {
        printf("Error: on read\n");
        // all nested ev_runs stop iterating
        ev_break(loop, EVBREAK_ALL);
    }

    // instead of printing the buffer, initiate a connection to all peers and
    // send the message across

    for (int i = 0; i < SERVER_COUNT; ++i)
    {
        if (send_to_peer(g_servers[i], 5555, buffer) < 0)
            return;
    }
}
