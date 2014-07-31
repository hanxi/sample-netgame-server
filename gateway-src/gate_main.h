#ifndef gate_main_h
#define gate_main_h

#include <lua.h>

int gate_init(int max, const char *host, int port, struct lua_State * l);
void gate_dispatch();
void gate_release();
 

#endif

