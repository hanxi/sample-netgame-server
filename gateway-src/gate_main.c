#include "gate_socket.h"
#include "gate_mq.h"
#include "databuffer.h"
#include "hashid.h"
#include "common.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h> 
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#define BACKLOG 32

struct connection {
    int id;        // gate_socket id
    char conn_tp;  // client = 'C', server = 'S', debugclient = 'D'
    char remote_name[32];
    struct databuffer buffer;
};

struct gate {
    int listen_id;
    int max_connection;
    struct hashid hash;
    int sid;       // server socket id
    struct connection *conn;
    struct messagepool mp;
    struct lua_State * l;
};

static struct gate * GATE = NULL;

struct gate *
gate_create(void) {
    struct gate * g = MALLOC(sizeof(*g));
    memset(g,0,sizeof(*g));
    g->listen_id = -1;
    return g;
}

static void
_gate_release(struct gate *g) {
    int i;
    for (i=0;i<g->max_connection;i++) {
        struct connection *c = &g->conn[i];
        if (c->id >=0) {
            gate_socket_close(c->id);
        }
    }
    if (g->listen_id >= 0) {
        gate_socket_close(g->listen_id);
    }
    messagepool_free(&g->mp);
    hashid_clear(&g->hash);
    FREE(g->conn);
    FREE(g);
}

static int
lua_dispatch(struct gate *g, int id, const char * buffer, int size) {
    lua_getglobal(g->l,"msg_dispatch");
    lua_pushnumber(g->l,id);
    lua_pushlstring(g->l,buffer,size);
	int err = lua_pcall(g->l, 2, 0, 0);
    if (err) {
		fprintf(stderr,"%s\n",lua_tostring(g->l,-1));
        return 1;
	}
    return 0;
}

static inline int
read_block(char * buffer, int pos) {
	int r = (int)buffer[pos] << 8 | (int)buffer[pos+1];
	return r;
}

static inline void
write_block(char * buffer, int r, int pos) {
	buffer[pos] = (r >> 8) & 0xff;
	buffer[pos+1] = r & 0xff;
}

static int
handshake(struct gate *g, struct connection *c, int id, char * buffer, int size) {
    // buffer : size | protId | device
    int need_free_buffer = 0;
    int dlen = size-4;
    char *device = MALLOC(dlen+1);
    memcpy(device,buffer+4,dlen);
    device[dlen] = '\0';

    lua_getglobal(g->l,"HANDSHAKE_CLIENT_MD5");
    const char * CLIENT_MD5 = lua_tostring(g->l,-1);
    lua_settop(g->l,0);
    lua_getglobal(g->l,"HANDSHAKE_SERVER_MD5");
    const char * SERVER_MD5 = lua_tostring(g->l,-1);
    lua_settop(g->l,0);
    lua_getglobal(g->l,"HANDSHAKE_DEBUG_MD5");
    const char * DEBUG_MD5 = lua_tostring(g->l,-1);
    lua_settop(g->l,0);

    printf("len:%d,%d\n",dlen,strlen(CLIENT_MD5));
    printf("device.handshake : %s\n",device);

    if (dlen==strlen(CLIENT_MD5)
        && strncmp(device,CLIENT_MD5,dlen)==0) {        // client connect to server
        printf("new client connect. id=%d\n",id);
        c->conn_tp = 'C';
    } else if (dlen==strlen(SERVER_MD5)
        && strncmp(device,SERVER_MD5,dlen)==0) { // server connect
        c->conn_tp = 'S';
        if (g->sid>0) {
            fprintf(stderr,"already connect:%s\n",c->remote_name);
            goto _clear;
        }
        printf("new server connect. id=%d\n",id);
        g->sid = id;
    } else if (dlen==strlen(DEBUG_MD5)
        && strncmp(device,DEBUG_MD5,dlen)==0) {
        printf("new debugclient connect. id=%d\n",id);
        c->conn_tp = 'D';
    } else {
        goto _clear;
    }
    return need_free_buffer;
_clear:
    databuffer_clear(&c->buffer,&g->mp);
    gate_socket_close(id);
    return need_free_buffer;
}

static int 
handlemsg(struct gate *g, struct connection *c, int id, char * buffer, int size) {
    // buffer : size | protId | id | -- 6byte
    int need_free_buffer = 0;
    int protId = read_block(buffer,2);
    int mid = read_block(buffer,4);
    switch (c->conn_tp) {
    case 'C':
        if (g->sid>0) {
            // write cid to buffer
            write_block(buffer,id,4);
            if(gate_socket_send(g->sid, buffer, size)>0) {
                return 1;
            }
            fprintf(stderr,"client send msg to server error.[protId:%d]\n",protId);
        } else {
            fprintf(stderr,"server already disconnect.\n");
            goto _clear;
        }
        break;
    case 'S':
        if (hashid_lookup(&g->hash, mid)>0) {
            // write sid to buffer
            write_block(buffer,g->sid,4);
            if(gate_socket_send(mid, buffer, size)>0) {
                return 1;
            }
            fprintf(stderr,"server send msg to client error.[protId:%d]\n",protId);
        } else {
            // tell server, client already disconnect
            int sz = 6;
            int protId = 98;
            int cid = mid;
            char * temp = MALLOC(sz);
            write_block(temp,sz,0);
            write_block(temp,protId,2);
            write_block(temp,cid,4);
            if(gate_socket_send(g->sid, temp, sz)<0) {
                fprintf(stderr,"client disconnect. send msg to server error.[protId:%d]\n",protId);
                FREE(temp);
            }
        }
        break;
    case 'D':
        lua_dispatch(g,id,buffer,size);
        break;
    default:
        fprintf(stderr,"unknown connection. id=%d.\n",id);
        goto _clear;
        break;
    }
    return need_free_buffer;
_clear:
    databuffer_clear(&c->buffer,&g->mp);
    gate_socket_close(id);
    return need_free_buffer;
}

static void
dispatch_message(struct gate *g, struct connection *c, int id, void * data, int sz) {
    databuffer_push(&c->buffer,&g->mp, data, sz);
    for (;;) {
        int size = databuffer_readheader(&c->buffer, &g->mp, 2);
        if (size < 0) {
            return;
        } else if (size > 0) {
            if (size >= 0x1000000) {
                fprintf(stderr, "Recv socket message > 16M");
                goto _clear;
            } else if (size>=4) { 
                char * temp = MALLOC(size+2);
                databuffer_read(&c->buffer,&g->mp,temp+2, size);

                write_block(temp,size,0);
                int protId = read_block(temp,2);

                printf("size=%d,protId=%d,id=%d\n",size,protId,id);

                int ret = 0;
                if (protId==100) {
                    ret = handshake(g,c,id,temp,size+2);
                } else if (size>=6) {
                    ret = handlemsg(g,c,id,temp,size+2);
                }
                // size<6 will ignore (if not handshake protocal)
                if (ret==0) {
                    FREE(temp);
                }
            }
            databuffer_reset(&c->buffer);
        }
    }
    return;
_clear:
    databuffer_clear(&c->buffer,&g->mp);
    gate_socket_close(id);
    return;
}

static void
dispatch_socket_message(struct gate *g, const struct gate_message * message) {
    switch(message->type) {
    case GATE_SOCKET_TYPE_DATA: {
        int id = hashid_lookup(&g->hash, message->id);
        if (id>=0) {
            struct connection *c = &g->conn[id];
            dispatch_message(g, c, message->id, message->buffer, message->ud);
        } else {
            fprintf(stderr, "Drop unknown connection %d message", message->id);
            gate_socket_close(message->id);
            FREE(message->buffer);
        }
        break;
    }
    case GATE_SOCKET_TYPE_CONNECT: {
        if (message->id == g->listen_id) {
            // start listening
            printf("start listening...\n");
            break;
        }
        int id = hashid_lookup(&g->hash, message->id);
        if (id>=0) {
            struct connection *c = &g->conn[id];
            printf("open %d %s\n",message->id,c->remote_name);
        } else {
            fprintf(stderr, "Close unknown connection %d\n", message->id);
            gate_socket_close(message->id);
        }
        break;
    }
    case GATE_SOCKET_TYPE_CLOSE:
    case GATE_SOCKET_TYPE_ERROR: {
        int id = hashid_remove(&g->hash, message->id);
        if (id>=0) {
            struct connection *c = &g->conn[id];
            databuffer_clear(&c->buffer,&g->mp);
            if (c->id==g->sid) {
                g->sid=-1;
            }
            memset(c, 0, sizeof(*c));
            c->id = -1;
            printf("%d close\n", message->id);
        }
        break;
    }
    case GATE_SOCKET_TYPE_ACCEPT:
        // report accept, then it will be get a GATE_SOCKET_TYPE_CONNECT message
        assert(g->listen_id == message->id);
        if (hashid_full(&g->hash)) {
            fprintf(stderr,"connection full\n");
            gate_socket_close(message->ud);
        } else {
            struct connection *c = &g->conn[hashid_insert(&g->hash, message->ud)];
            int sz = sizeof(c->remote_name) - 1;
            c->id = message->ud;
            memcpy(c->remote_name, message->buffer, sz);
            c->remote_name[sz] = '\0';
            gate_socket_start(message->ud);
        }
        break;
    }
}

static int
start_listen(struct gate *g, const char * host, int port) {
    g->listen_id = gate_socket_listen(host, port, BACKLOG);
    if (g->listen_id < 0) {
        return 1;
    }
    gate_socket_start(g->listen_id);
    printf("listening %d\n",g->listen_id);
    return 0;
}

int
gate_init(int max, const char * host, int port, struct lua_State *l) {
    struct gate *g = gate_create();

    g->l = l;

    hashid_init(&g->hash, max);

    g->conn = MALLOC(max * sizeof(struct connection));
    memset(g->conn, 0, max *sizeof(struct connection));
    g->max_connection = max;

    int i;
    for (i=0;i<max;i++) {
        g->conn[i].id = -1;
    }

    GATE = g;
    return start_listen(GATE,host,port);
}

void 
gate_dispatch() {
	struct gate_message msg;
	while (!gate_mq_pop(&msg)) {
        dispatch_socket_message(GATE, &msg);
	}
}

void
gate_release() {
    _gate_release(GATE);
}

