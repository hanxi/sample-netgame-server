CFLAGS = -g -Wall
SHARED = -fPIC --shared
INCLS = -I3rd/socket-server -Icommon-src

LUA_STATICLIB := 3rd/lua/liblua.a
LUA_LIB ?= $(LUA_STATICLIB)
LUA_INC ?= 3rd/lua

LUA_CLIB = clientsocket md5 socket lproto
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
	cp 3rd/lproto/lualib/lproto.lua lualib/
	cp bin.server/scripts/prot.lua bin.client/

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

$(LUA_CLIB_PATH)/md5.so : 3rd/lua-md5/md5.c 3rd/lua-md5/md5lib.c 3rd/lua-md5/compat-5.2.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) -I3rd/lua-md5 $^ -o $@

$(LUA_CLIB_PATH)/socket.so: lualib-src/lua-socket.c common-src/socket_mq.c common-src/client_socket.c 
	$(CC) $(CFLAGS) $(SHARED) -o $@ $^ $(INCLS) -I$(LUA_INC)

$(LUA_CLIB_PATH)/lproto.so: 3rd/lproto/src/lproto.c 
	$(CC) $(CFLAGS) $(SHARED) -o $@ $^ $(INCLS) -I$(LUA_INC)

### gateway
GATEWAY_SRC = gate_mq.c gate_socket.c gate_main.c main.c
$(BIN_GATEWAY_PATH)/gateway : $(foreach v, $(GATEWAY_SRC), gateway-src/$(v)) 3rd/socket-server/socket_server.c $(LUA_LIB)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLS) -I$(LUA_INC) -Wl,-E -lm -ldl -lrt

### server
$(BIN_SERVER_PATH)/server : server-src/main.c common-src/socket_mq.c common-src/client_socket.c $(LUA_LIB)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLS) -I$(LUA_INC) -Wl,-E -lm -ldl -lrt


clean:
	rm -f $(BIN_GATEWAY_PATH)/gateway $(BIN_SERVER_PATH)/server $(LUA_CLIB_PATH)/*.so

