#ifndef client_socket_h
#define client_socket_h

#include <netinet/in.h>
#include <lua.h>

struct socket;

int socket_getfd(struct socket * s);

struct socket * socket_new();

void socket_delete(struct socket * s);

int socket_connect(const char * addr, int port, struct socket * s);

int64_t socket_send(struct socket *s, const void * buffer, int sz);

int socket_send_remainbuffer(struct socket *s);

typedef int (*dispatch_cb)(lua_State *L, struct socket *s, const char * buffer, int size);
void socket_msgdispatch(struct socket * s, dispatch_cb, void *L);

#endif

