#include "client_socket.h"
#include "common.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h> 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static int
lsendbuffer(lua_State * L) {
    struct socket * s = lua_touserdata(L,1);
    void * buffer = lua_touserdata(L,2);
    int sz = lua_tonumber(L,3);
    int ret = socket_send(s,buffer, sz);
    lua_settop(L,0);
    lua_pushnumber(L,ret);
	return 1;
}

static int
lsendstring(lua_State * L) {
    struct socket * s = lua_touserdata(L,1);
    size_t sz=0;
    const char * str = lua_tolstring(L,2,&sz);
    void * buffer = MALLOC(sz);
    memcpy(buffer,str,sz);
    int ret = socket_send(s,buffer, sz);
    lua_settop(L,0);
    lua_pushnumber(L,ret);
	return 1;
}

static int
lgetfd(lua_State * L) {
    struct socket * s = lua_touserdata(L,1);
    lua_settop(L,0);
    int fd = socket_getfd(s);
    lua_pushnumber(L,fd);
    return 1;
}

static int 
lconnect(lua_State * L) {
	const char * addr = luaL_checkstring(L, 1);
	int port = luaL_checkinteger(L, 2);
    struct socket * s = socket_new();
    int ret = socket_connect(addr, port,s);
	if (ret) {
		return luaL_error(L, "Connect %s %d failed", addr, port);
	}
    lua_pushlightuserdata(L,s);
	return 1;
}

static inline int
read_block(const char * buffer, int pos) {
    int r = (int)buffer[pos] << 8 | (int)buffer[pos+1];
    return r;
}

static int
lua_dispatch(lua_State *L, struct socket *s, char * buffer, int size) {
    int err = 0;
    int fd = socket_getfd(s);
    if (size==0) {
        lua_getglobal(L,"disconnect_dispatch");
        lua_pushnumber(L,fd);
        err = lua_pcall(L, 1, 0, 0);
    } else {
        lua_getglobal(L,"msg_dispatch");
        lua_pushnumber(L,fd);
        lua_pushlightuserdata(L,buffer);
        lua_pushnumber(L,size);
        err = lua_pcall(L, 3, 0, 0);
    }
    if (err) {
        fprintf(stderr,"%s\n",lua_tostring(L,-1));
        return 1;
    }
    return 0;
}

static int
lsocketloop(lua_State * L) {
    struct socket * s = lua_touserdata(L,1);
    socket_send_remainbuffer(s);
    socket_msgdispatch(s,lua_dispatch,L);
    return 1;
}

static int
ldisconnect(lua_State * L) {
    struct socket * s = lua_touserdata(L,1);
    socket_delete(s);
    return 0;
}

int
luaopen_socket_core(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
        {"connect",lconnect},
        {"disconnect",ldisconnect},
        {"sendbuffer",lsendbuffer},
        {"sendstring",lsendstring},
        {"getfd",lgetfd},
        {"loop",lsocketloop},
		{NULL,NULL},
    };
	luaL_newlib(L,l);
    return 1;
}

