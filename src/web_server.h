#ifndef SUPER_ROCKET_WEB_SERVER_H
#define SUPER_ROCKET_WEB_SERVER_H

#include <stddef.h>

/*
 * Builds the reply to one request head in head (len bytes) into out.
 * Returns the reply length, 0 while the head is incomplete, or -1 when out is
 * too small. *streaming is reserved and currently always set to zero.
 */
int web_respond(const char *head, size_t len, char *out, size_t size, int *streaming);

/* Opens a listening socket on addr:port.  Returns the descriptor or -1. */
int web_server_listen(const char *addr, int port);

/*
 * Accepts connections, reads requests, sends replies and streams frames for
 * at most timeout seconds.
 */
void web_server_service(int listen_fd, double timeout);

void web_server_shutdown(int listen_fd);

#endif
