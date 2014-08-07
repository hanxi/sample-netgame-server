#include "client_socket.h"
#include "common.h"
#include "databuffer.h"
#include "socket_mq.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

#define MIN_READ_BUFFER 64
#define SOCKET_CLOSE 1

struct write_buffer {
    struct write_buffer * next;
    char *ptr;
    int sz;
    void *buffer;
};

struct socket {
    int fd;
    int size;
    int64_t wb_size;
    struct write_buffer * head;
    struct write_buffer * tail;
    struct databuffer read_buffer;
    struct messagepool mp;
    struct message_queue * q;
};

static void
force_close(struct socket *s) {
    struct write_buffer *wb = s->head;
    while (wb) {
        struct write_buffer *tmp = wb;
        wb = wb->next;
        FREE(tmp->buffer);
        FREE(tmp);
    }
    s->head = s->tail = NULL;
    databuffer_clear(&s->read_buffer, &s->mp);
    messagepool_free(&s->mp);
    close(s->fd);
    if (s->q) {
        socket_mq_delete(s->q);
    }
    s->q = NULL;
}

int 
socket_getfd(struct socket * s) {
    return s->fd;
}

struct socket *
socket_new() {
    struct socket * s = (struct socket *)MALLOC(sizeof(struct socket));
    s->fd = -1;
    s->size = MIN_READ_BUFFER;
    s->wb_size = 0;
    s->head = NULL;
    s->tail = NULL;
	memset(&s->read_buffer, 0, sizeof(s->read_buffer));
	memset(&s->mp, 0, sizeof(s->mp));
    return s;
}

void 
socket_delete(struct socket * s) {
    force_close(s);
    FREE(s);
}

int
socket_connect(const char * addr, int port, struct socket * s) {
	struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	char _port[16];
	sprintf(_port, "%d", port);
    int status = getaddrinfo(addr, _port, &hints, &servinfo);
    if (status!=0) {
        fprintf(stderr, "[error] Connect %s %d failed\n", addr, port);
        freeaddrinfo(servinfo);
        return 1;
    }

    int fd;
    struct addrinfo *p;
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((fd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            fprintf(stderr,"[error] client: socket\n");
            continue;
        }

        if (connect(fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(fd);
            fprintf(stderr,"[error] client: connect\n");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

	int flag = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flag | O_NONBLOCK);

    s->fd = fd;
    s->size = MIN_READ_BUFFER;
    s->wb_size = 0;
    s->head = NULL;
    s->tail = NULL;
    s->q = socket_mq_new();

    return 0;
}

int
socket_send_remainbuffer(struct socket *s) {
    if (s==NULL || s->fd==-1) {
        return -1;
    }
    while (s->head) {
        struct write_buffer * tmp = s->head;
        for (;;) {
            int sz = write(s->fd, tmp->ptr, tmp->sz);
            if (sz < 0) {
                switch(errno) {
                case EINTR:
                    continue;
                case EAGAIN:
                    return 0;
                }
                force_close(s);
                return SOCKET_CLOSE;
            }
            s->wb_size -= sz;
            if (sz != tmp->sz) {
                tmp->ptr += sz;
                tmp->sz -= sz;
                return 0;
            }
            break;
        }
        s->head = tmp->next;
        FREE(tmp->buffer);
        FREE(tmp);
    }
    s->tail = NULL;

    return 0;
}

static void
append_sendbuffer(struct socket *s, const void * buffer, int sz, int n) {
    struct write_buffer * buf = MALLOC(sizeof(*buf));
    buf->ptr = (char *)buffer+n;
    buf->sz = sz - n;
    buf->buffer = (void *)buffer;
    buf->next = NULL;
    s->wb_size += buf->sz;
    if (s->head == NULL) {
        s->head = s->tail = buf;
    } else {
        assert(s->tail != NULL);
        assert(s->tail->next == NULL);
        s->tail->next = buf;
        s->tail = buf;
    }
}

// return -1 when error
int64_t
socket_send(struct socket *s, const void * buffer, int sz) {
    if (s==NULL || s->fd==-1) {
        fprintf(stderr, "erro . socket_send. %d\n",(int)s);
        return -1;
    }
    if (s->head == NULL) {
        int n = write(s->fd, buffer, sz);
        if (n<0) {
            switch(errno) {
            case EINTR:
            case EAGAIN:
                n = 0;
                break;
            default:
                fprintf(stderr, "socket: write to (fd=%d) error.\n",s->fd);
                force_close(s);
                return -1;
            }
        }
        if (n == sz) {
            return sz;
        }
        append_sendbuffer(s, buffer, sz, n);
    } else {
        append_sendbuffer(s, buffer, sz, 0);
    }
    return s->wb_size;
}

// return -1 (ignore) when error
static int
recv_msg(struct socket *s, struct socket_message * result) {
    if (s==NULL || s->fd==-1) {
        return -1;
    }
    int sz = s->size;
    char * buffer = MALLOC(sz);
    int n = (int)read(s->fd, buffer, sz);
    if (n<0) {
        FREE(buffer);
        switch(errno) {
        case EINTR:
            break;
        case EAGAIN:
            //fprintf(stderr, "socket-server: EAGAIN capture.\n");
            break;
        default:
            // close when error
            force_close(s);
            return SOCKET_CLOSE;
        }
        return -1;
    }
    if (n==0) {
        FREE(buffer);
        force_close(s);
        return SOCKET_CLOSE;
    }

    if (n == sz) {
        s->size *= 2;
    } else if (sz > MIN_READ_BUFFER && n*2 < sz) {
        s->size /= 2;
    }

    result->sz = n;
    result->data = buffer;
    return 0;
}

static inline void
write_block(char * buffer, int r, int pos) {
    buffer[pos] = (r >> 8) & 0xff;
    buffer[pos+1] = r & 0xff;
}

void
socket_msgdispatch(struct socket * s, dispatch_cb cb, void * L) {
    if (s==NULL || s->fd==-1 || s->q==NULL) {
        return;
    }
    struct socket_message sm;
    int ret = recv_msg(s, &sm);
    if (ret==0) {
        socket_mq_push(s->q,&sm);
    } else if (ret==SOCKET_CLOSE) {
        cb(L,s,NULL,0);
        s->fd = -1;
        return;
    }
    struct socket_message msg;
    while (!socket_mq_pop(s->q,&msg)) {
        databuffer_push(&s->read_buffer,&s->mp, msg.data, msg.sz);
        int size = databuffer_readheader(&s->read_buffer, &s->mp, 2);
        if (size > 0) {
            if (size >= 0x1000000) {
                fprintf(stderr, "Recv socket message > 16M");
                databuffer_clear(&s->read_buffer,&s->mp);
            } else if (size>=2) {
                char * temp = MALLOC(size);
                databuffer_read(&s->read_buffer,&s->mp,temp, size);
                cb(L,s,temp,size);
                FREE(temp);
            }
            databuffer_reset(&s->read_buffer);
        }
    }
}

