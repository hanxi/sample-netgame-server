#include "gate_socket.h"
#include "socket_server.h"
#include "gate_mq.h"
#include "common.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#define GATE_SOURCE 200

static struct socket_server * SOCKET_SERVER = NULL;

void
gate_socket_init() {
    SOCKET_SERVER = socket_server_create();
}

void
gate_socket_exit() {
    socket_server_exit(SOCKET_SERVER);
}

void
gate_socket_free() {
    socket_server_release(SOCKET_SERVER);
    SOCKET_SERVER = NULL;
}

static void
forward_message(int type, struct socket_message * result) {
    struct gate_message *sm;
    int sz = sizeof(*sm);
    sm = (struct gate_message *)MALLOC(sz);
    sm->type = type;
    sm->id = result->id;
    sm->ud = result->ud;
    sm->buffer = result->data;

    gate_mq_push(sm);
}

int
gate_socket_poll() {
    struct socket_server *ss = SOCKET_SERVER;
    assert(ss);
    struct socket_message result;
    int more = 1;
    int type = socket_server_poll(ss, &result, &more);
    switch (type) {
    case SOCKET_EXIT:
        return 0;
    case SOCKET_DATA:
        forward_message(GATE_SOCKET_TYPE_DATA, &result);
        break;
    case SOCKET_CLOSE:
        forward_message(GATE_SOCKET_TYPE_CLOSE, &result);
        break;
    case SOCKET_OPEN:
        forward_message(GATE_SOCKET_TYPE_CONNECT, &result);
        break;
    case SOCKET_ERROR:
        forward_message(GATE_SOCKET_TYPE_ERROR, &result);
        break;
    case SOCKET_ACCEPT:
        forward_message(GATE_SOCKET_TYPE_ACCEPT, &result);
        break;
    default:
        fprintf(stderr, "Unknown socket message type %d.\n",type);
        return -1;
    }
    if (more) {
        return -1;
    }
    return 1;
}

int
gate_socket_send(int id, void *buffer, int sz) {
    int64_t wsz = socket_server_send(SOCKET_SERVER, id, buffer, sz);
    if (wsz < 0) {
        FREE(buffer);
        return -1;
    } else if (wsz > 1024 * 1024) {
        int kb4 = wsz / 1024 / 4;
        if (kb4 % 256 == 0) {
            fprintf(stderr, "%d Mb bytes on socket %d need to send out", (int)(wsz / (1024 * 1024)), id);
        }
    }
    return 0;
}

//void
//gate_socket_send_lowpriority(int id, void *buffer, int sz) {
//    socket_server_send_lowpriority(SOCKET_SERVER, id, buffer, sz);
//}

int
gate_socket_listen(const char *host, int port, int backlog) {
    return socket_server_listen(SOCKET_SERVER, GATE_SOURCE, host, port, backlog);
}

int
gate_socket_connect(const char *host, int port) {
    return socket_server_connect(SOCKET_SERVER, GATE_SOURCE, host, port);
}

int
gate_socket_bind(int fd) {
    return socket_server_bind(SOCKET_SERVER, GATE_SOURCE, fd);
}

void
gate_socket_close(int id) {
    socket_server_close(SOCKET_SERVER, GATE_SOURCE, id);
}

void
gate_socket_start(int id) {
    socket_server_start(SOCKET_SERVER, GATE_SOURCE, id);
}

//void
//gate_socket_nodelay(int id) {
//    socket_server_nodelay(SOCKET_SERVER, id);
//}
