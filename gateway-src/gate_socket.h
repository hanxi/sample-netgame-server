#ifndef gate_socket_h
#define gate_socket_h

#define GATE_SOCKET_TYPE_DATA 1
#define GATE_SOCKET_TYPE_CONNECT 2
#define GATE_SOCKET_TYPE_CLOSE 3
#define GATE_SOCKET_TYPE_ACCEPT 4
#define GATE_SOCKET_TYPE_ERROR 5

void gate_socket_init();
void gate_socket_exit();
void gate_socket_free();
int gate_socket_poll();

int gate_socket_send(int id, void *buffer, int sz);
//void gate_socket_send_lowpriority(int id, void *buffer, int sz);
int gate_socket_listen(const char *host, int port, int backlog);
int gate_socket_connect(const char *host, int port);
int gate_socket_bind(int fd);
void gate_socket_close(int id);
void gate_socket_start(int id);
//void gate_socket_nodelay(int id);

#endif
