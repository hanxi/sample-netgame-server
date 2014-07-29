CFLAGS = -g -Wall
SHARED = -fPIC --shared
INCLS = -I3rd/socket-server

LUA_STATICLIB := 3rd/lua/liblua.a
LUA_LIB ?= $(LUA_STATICLIB)
LUA_INC ?= 3rd/lua

LUA_CLIB = clientsocket
LUA_CLIB_PATH ?= luaclib
BIN_GATEWAY_PATH ?= bin.gateway
BIN_SERVER_PATH ?= bin.server

all : \
	$(LUA_CLIB_PATH) \
	$(BIN_GATEWAY_PATH) \
	$(BIN_SERVER_PATH) \
	$(BIN_GATEWAY_PATH)/gateway \
	$(BIN_SERVER_PATH)/server \
	$(foreach v, $(LUA_CLIB), $(LUA_CLIB_PATH)/$(v).so) 

$(LUA_STATICLIB) :
	cd 3rd/lua && $(MAKE) CC=$(CC) linux

$(LUA_CLIB_PATH) :
	mkdir $(LUA_CLIB_PATH)

$(BIN_GATEWAY_PATH) :
	mkdir $(BIN_GATEWAY_PATH)

$(BIN_SERVER_PATH) :
	mkdir $(BIN_SERVER_PATH)

### lualib-src
$(LUA_CLIB_PATH)/clientsocket.so: lualib-src/lua-clientsocket.c
	$(CC) $(CFLAGS) $(SHARED) -o $@ $^ $(INCLS) -I$(LUA_INC) -lpthread

### gateway
GATEWAY_SRC = gate_mq.c gate_socket.c main.c
$(BIN_GATEWAY_PATH)/gateway : $(foreach v, $(GATEWAY_SRC), gateway-src/$(v)) 3rd/socket-server/socket_server.c $(LUA_LIB)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLS) -I$(LUA_INC) -Wl,-E -lpthread -lm -ldl -lrt 

### server
$(BIN_SERVER_PATH)/server : server-src/main.c 3rd/socket-server/socket_server.c
	$(CC) $(CFLAGS) -o $@ $^ $(INCLS)

clean:
	rm -f $(BIN_GATEWAY_PATH)/gateway $(BIN_SERVER_PATH)/server $(LUA_CLIB_PATH)/*.so

