#include "gate_socket.h"
#include "gate_mq.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#define MALLOC malloc
#define FREE free

static struct lua_State *L = NULL;

/*
static void *
_poll(void * ud) {
    struct socket_server *ss = ud;
    struct socket_message result;
    for (;;) {
        int type = socket_server_poll(ss, &result, NULL);
        int i=0;
        switch (type) {
        case SOCKET_EXIT:
            return NULL;
        case SOCKET_DATA:
            printf("message(%u) [id=%d] size=%d\n",result.opaque,result.id,result.ud);
            printf("data:");
            for (i=0;i<result.ud; i++) {
                printf("%x ",result.data[i]);
            }
            printf("\n");
            free(result.data);
            break;
        case SOCKET_CLOSE:
            printf("close(%u) [id=%d]\n",result.opaque,result.id);
            break;
        case SOCKET_OPEN:
            printf("open(%u) [id=%d] %s\n",result.opaque,result.id,result.data);
            break;
        case SOCKET_ERROR:
            printf("error(%u) [id=%d]\n",result.opaque,result.id);
            break;
        case SOCKET_ACCEPT:
            printf("accept(%u) [id=%d %s] from [%d]\n",result.opaque, result.ud, result.data, result.id);
            socket_server_start(ss,300,result.ud);
            break;
        default:
            printf("unknow type:%d\n",type);
        }
    }
}
*/

static int
lua_dispatch(struct gate_message * msg) {
    lua_getglobal(L,"msg_dispatch");
    lua_pushnumber(L,msg->type);
    lua_pushnumber(L,msg->id);
    lua_pushnumber(L,msg->ud);
    lua_pushstring(L,msg->buffer);
	int err = lua_pcall(L, 4, 0, 0);
    if (err) {
		fprintf(stderr,"%s\n",lua_tostring(L,-1));
        return 1;
	}
    return 0;
}

void 
gate_dispatch() {
	struct gate_message msg;
	while (!gate_mq_pop(&msg)) {
        if (lua_dispatch(&msg)) {
            FREE(msg.buffer);
        }
	}
}

int
sigign() {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);
    return 0;
}

static void *
gate_loop() {
    for (;;) {
        int r = gate_socket_poll();
        if (r==0) {
            break;
        }
        if (r<0) {
            continue;
        }
        gate_dispatch();
    }
    return NULL;
}

static const char * lua_config = "\
    local root=...\
    print('root:',root)\
    package.path=root..'/scripts/?.lua;'\
    require('main')\
";

static void
lua_init(const char * root) {
    L = luaL_newstate();
    luaL_openlibs(L);   // link lua lib

    int err = luaL_loadstring(L, lua_config);
    assert(err == LUA_OK);
    lua_pushstring(L, root);
    err = lua_pcall(L, 1, 0, 0);
    if (err) {
        fprintf(stderr,"%s\n",lua_tostring(L,-1));
        lua_close(L);
        exit(1);
    }
}

int
main(int argc, char * argv[]) {
    sigign();

    char root[1024] = {0};
    int i = (int)strlen(argv[0])-1;
    for(;(argv[0][i]!='/'&&(argv[0][i]!='\\'));i--){;}
    memset(root,0,i+1);
    memcpy(root,argv[0],i);

    lua_init(root);

    gate_mq_init();
    gate_socket_init();

    int id = gate_socket_listen("",8888,32);
    printf("listening %d\n",id);
    gate_socket_start(id);
    gate_loop();
    gate_socket_free();

    return 0;
}
