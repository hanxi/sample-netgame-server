#include "common.h"

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

static struct lua_State *L = NULL;

static int 
call_lualoop() {
    lua_getglobal(L,"loop");
    int err = lua_pcall(L, 0, 0, 0);
    if (err) {
        fprintf(stderr,"%s\n",lua_tostring(L,-1));
        return 1;
    }
    return 0;
}

static void
main_loop() {
    for (;;) {
        call_lualoop();
        usleep(2000); // sleep 2ms
    }
}

static const char * lua_config = "\
    local root=...\
    print('root:',root)\
    package.path = root..'/scripts/?.lua;'..root..'/../lualib/?.lua;' \
    package.cpath = root..'/../luaclib/?.so;' \
    require('main')\
";

static void
lua_init(const char * root) {
    L = luaL_newstate();
	luaL_checkversion(L);
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

    main_loop();

    return 0;
}

