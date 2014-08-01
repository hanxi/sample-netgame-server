#include "client_socket.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

static struct lua_State *L = NULL;
static struct socket * SOCKET = NULL;

static int
lua_dispatch(const char * buffer, int size) {
    lua_getglobal(L,"msg_dispatch");
    lua_pushlstring(L,buffer,size);
    int err = lua_pcall(L, 1, 0, 0);
    if (err) {
        fprintf(stderr,"%s\n",lua_tostring(L,-1));
        return 1;
    }
    return 0;
}

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
main_loop(struct socket * s) {
    for (;;) {
        socket_msgdispatch(s,lua_dispatch);
        socket_send_remainbuffer(s);
        call_lualoop();
    }
}

static const char * lua_config = "\
    local root=...\
    print('root:',root)\
    package.path=root..'/scripts/?.lua;'\
    require('main')\
";

static int 
lconnect(lua_State * L) {
	const char * addr = luaL_checkstring(L, 1);
	int port = luaL_checkinteger(L, 2);
    int ret = socket_connect(addr, port,&SOCKET);
	if (ret) {
		return luaL_error(L, "Connect %s %d failed", addr, port);
	}
	return 0;
}

static int
lsend(lua_State * L) {
	size_t sz = 0;
	const char * msg = luaL_checklstring(L, 1, &sz);
    int ret = socket_send(SOCKET, msg, sz);
    lua_settop(L,0);
    lua_pushnumber(L,ret);
	return 1;
}

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
    // regist c function to lua
    lua_register(L,"connect",lconnect);
    lua_register(L,"send",lsend);
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

    main_loop(SOCKET);

    return 0;
}
