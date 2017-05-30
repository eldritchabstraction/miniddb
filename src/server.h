#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>

/*
 * The server independently listens for new connections and passes messages to
 * be processed by main thread
 */
void server_start(uint16_t bind_port);

#endif // SERVER_H
