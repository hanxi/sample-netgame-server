#include "client_socket.h"
#include "socket_mq.h"
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
        usleep(2000); // sleep 2ms
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
    int ret = socket_connect(addr, port,SOCKET);
	if (ret) {
		return luaL_error(L, "Connect %s %d failed", addr, port);
	}
	return 1;
}

static int
lsend(lua_State * L) {
	size_t sz = 0;
	const char * msg = luaL_checklstring(L, 1, &sz);
    printf("msg:%s\n",msg);
    int ret = socket_send(SOCKET,msg, sz);
    printf("msg:%s\n",msg);
    lua_settop(L,0);
    lua_pushnumber(L,ret);
	return 1;
}

int 
lsocket(lua_State *L) {
	luaL_Reg l[] = {
		{"connect", lconnect},
		{"send", lsend},
		{NULL,NULL},
	};
	luaL_newlib(L,l);
	return 1;
}

static void
checkluaversion(lua_State *L) {
	const lua_Number *v = lua_version(L);
	if (v != lua_version(NULL))
		fprintf(stderr,"multiple Lua VMs detected\n");
	else if (*v != LUA_VERSION_NUM) {
		fprintf(stderr,"Lua version mismatch: app. needs %f, Lua core provides %f\n",
			(double)LUA_VERSION_NUM, (double)*v);
	}
}

static void
lua_init(const char * root) {
    L = luaL_newstate();
	checkluaversion(L);
    luaL_openlibs(L);   // link lua lib

	luaL_requiref(L, "socket", lsocket, 0);

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

    socket_mq_init();
    SOCKET = socket_new();

    lua_init(root);

    main_loop(SOCKET);

    socket_delete(SOCKET);
    socket_mq_release();

    return 0;
}

