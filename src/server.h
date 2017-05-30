#ifndef SERVER_H
#define SERVER_H


#if defined (__cplusplus)
extern "C" {
#endif

/*
 * The server independently listens for new connections and passes messages to
 * be processed by main thread
 */
void server_start(int bind_port);


#if defined (__cplusplus)
}
#endif

#endif // SERVER_H
