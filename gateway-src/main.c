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


    gate_init(MAX_CONNECTION, "127.0.0.1",8888,L);

    printf("0\n");
    gate_loop();

    printf("1\n");
    gate_release();
    printf("2\n");
    gate_socket_free();
    printf("3\n");
    gate_mq_release();
    printf("4\n");

    return 0;
}
