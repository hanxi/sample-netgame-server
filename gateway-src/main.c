#include "gate_socket.h"
#include "gate_mq.h"
#include "gate_main.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <time.h>

#define MAX_CONNECTION 10240

static struct lua_State *L = NULL;

static void
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
        usleep(2000); // sleep 2ms
    }
}

static const char * lua_config = "\
    local root=...\
    print('root path:',root)\
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
sigign() {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);
    return 0;
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

    lua_getglobal(L,"CONFIG_IP");
    const char * IP = lua_tostring(L,-1);
    lua_settop(L,0);
    lua_getglobal(L,"CONFIG_PORT");
    int PORT = lua_tointeger(L,-1);
    lua_settop(L,0);
    printf("[host %s:%d ] staring gateway ...\n",IP,PORT);
    gate_init(MAX_CONNECTION, IP,PORT,L);
    gate_loop();
    gate_release();

    gate_socket_free();
    gate_mq_release();

    return 0;
}
