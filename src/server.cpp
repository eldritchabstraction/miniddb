#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for read
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <errno.h>

#include "server.h"

#include <vector>

#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(fmt, args...) printf("DEBUG: " fmt, ##args)
#define DEBUG_PRINT_CLEAN(fmt, args...) printf(fmt, ##args)
#else
#define DEBUG_PRINT(x) do {} while(0)
#define DEBUG_PRINT_CLEAN do{} while(0)
#endif

#define ERROR_PRINT(fmt, args...) printf("ERROR: " fmt, ##args)

class poll_algo_t;
class listening_socket_t;

class listening_socket_t
{
public:
    listening_socket_t(int fd, int bind): fd_(fd), bind_(bind) {}
    void init_l_sock(void);
    int accept_incoming_connections(poll_algo_t &a_poll);
    int fd(void) { return fd_; }

private:
    int fd_;
    int bind_; // bind_port
};

class poll_algo_t
{
public:
    poll_algo_t(int timeout = 3 * 60 * 1000)
    {
        timeout_ = timeout;

        kill_server = 0;
        kill_connection = 0;
    }

    int timeout(void) { return timeout_; }

    int polling (std::vector<int> &readable_fds);

    void add_connection(int fd);

    int kill_server, kill_connection;
private:
    std::vector<struct pollfd> fds_;
    int timeout_;
    // algorithm status
};

/*
 * poll_algo_add_connection:
 */
void poll_algo_t::add_connection(int fd)
{
    DEBUG_PRINT("Adding new connection %d\n", fd);
    struct pollfd *temp = new struct pollfd;
    temp->fd = fd;
    temp->events = POLLIN;

    fds_.push_back(*temp);
}

/*
 * poll: not only poll but process status
 */

int poll_algo_t::polling (std::vector<int> &readable_fds)
{
    DEBUG_PRINT("fds_ size %d\n", (int)fds_.size());
    int rc = poll(&fds_[0], fds_.size(), timeout_);

    if (rc < 0)
    {
        ERROR_PRINT("poll failed: %s\n", strerror(errno));
        return rc;
    }
    else if (rc == 0)
    {
        DEBUG_PRINT("poll timed out\n");
        return rc;
    }

    // iterate through fds and find all revent POLLIN
    std::vector<struct pollfd>::iterator it = fds_.begin();
    for (; it != fds_.end(); ++it)
    {
        struct pollfd cursor = *it;
        if (cursor.revents == 0)
        {
            continue;
        }
        else if (cursor.revents != POLLIN)
        {
            ERROR_PRINT("Unexpected revents value: %d\n", cursor.revents);
            kill_server = 1;
            return -1;
        }

        readable_fds.push_back(cursor.fd);
    }
    return rc;
}

/*
 * init_l_sock: initializes listening socket
 * currently configuration options aren't passed making this a rather unmodular
 * routine
 */
void listening_socket_t::init_l_sock(void)
{
    if ((fd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        ERROR_PRINT("opening socket: %s\n", strerror(errno));
        exit(1);
    }

    DEBUG_PRINT("Listening socket on %d\n", fd_);

    // Allow socket descriptor to be reusable
    int on = 1;
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, (char *)(&on), sizeof(on)) < 0)
    {
        ERROR_PRINT("setsockopt failed: %s\n", strerror(errno));
        close(fd_);
        exit(1);
    }

    // Set socket to be nonblocking, connection sockets inherit from listening
    // socket

    if (ioctl(fd_, FIONBIO, (char*)(&on)) < 0)
    {
        ERROR_PRINT("ioctl failed\n");
        close(fd_);
        exit(1);
    }

    // Initialize socket structure
    struct sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_addr.s_addr = INADDR_ANY;
    server_sockaddr.sin_port = htons(bind_);

    // Bind to socket
    if (bind(fd_, (struct sockaddr *)&server_sockaddr,
        sizeof(server_sockaddr)) < 0)
    {
        ERROR_PRINT("on bind: %s\n", strerror(errno));
        close(fd_);
        exit(1);
    }

    // Listen on socket
    if (listen(fd_, 32) < 0)
    {
        ERROR_PRINT("listen failed: %s\n", strerror(errno));
        close(fd_);
        exit(1);
    }
}

/*
 * accept_incoming_connections:
 */
int listening_socket_t::accept_incoming_connections(poll_algo_t &a_poll)
{
    while(1)
    {
        int connection_sockfd = accept(fd_, NULL, NULL);

        if (connection_sockfd < 0)
        {
            if (errno != EWOULDBLOCK)
            {
                ERROR_PRINT("accept failed: %s", strerror(errno));
                return -1;
            }

            return 0;
        }

        a_poll.add_connection(connection_sockfd);
    }
}

void server_start(int bind_port)
{
    // Get a socket structure
    listening_socket_t l_sock(-1, bind_port);

    l_sock.init_l_sock();

    // Initialize poll fd array
    poll_algo_t a_poll;

    // Assign listening socket to poll fd array
    a_poll.add_connection(l_sock.fd());

    std::vector<int> readable_fds;
    do
    {
        DEBUG_PRINT("Waiting on poll for 3 minutes\n");
        // poll and return a readable fd
        if (a_poll.polling(readable_fds) <= 0)
            break;

        // process readable fds into either new connections or new reads
        std::vector<int>::iterator it = readable_fds.begin();
        for (; it != readable_fds.end(); it++)
        {
            int socket_fd = *it;
            if (socket_fd == l_sock.fd())
            {
                DEBUG_PRINT("Listening socket is readable\n");
                // ELDR: I really don't like the idea of passing the poll object
                // into the listen socket, but I need to add connections. Another
                // way to do this is to have poll be a singleton

                if (l_sock.accept_incoming_connections(a_poll) < 0)
                {
                    // kill server?
                    ERROR_PRINT("accept failed, breaking\n");
                    break;
                }
            }
            else
            {
                DEBUG_PRINT("Connection socket %d is readable\n", socket_fd);

                while(1)
                {
                    char buffer[256];
                    memset(buffer, 0, sizeof(buffer));
                    int rc = recv(socket_fd, buffer, sizeof(buffer), 0);

                    if (rc < 0)
                    {
                        if (errno != EWOULDBLOCK)
                        {
                            ERROR_PRINT("recv failed: %s\n", strerror(errno));
                            close(socket_fd);
                        }
                        break;
                    } else if (rc == 0) {
                        DEBUG_PRINT("connection closed\n");
                        close(socket_fd);
                        break;
                    }

                    DEBUG_PRINT("%d bytes received\n", rc);
                    DEBUG_PRINT("buffer: %s\n", buffer);

                    break;
                }
            }
            /*
            if (close_connection)
            {
                close(a_poll.fds()[i].fd);
                a_poll.fds()[i].fd = -1;
                compress_array = 1;
            }
            */
        }
        // TODO: check for kill_server
   } while (1);

}
