
## simple network game server

> ### gateway

> ### logic server

> ### console client

## test in linux

* compile


    $ make


* start gateway


    $ ./bin.gateway/gateway


* start client


    $ ./bin.server/server
    

* start client


    $ lua ./bin.client/client.lua


### about test

1. start gateway, wait message from client or server

2. start server, connect to gateway, gateway set server and save server fd

3. start client, connect to gateway and connect server, send others message to server

4. if you want connect to gateway as debugclient, change client handshake md5 string for "debug_md5" in client.lua

## TODO :

> ### net protocal


## others :

1. 3rd/cstring was copy from https://github.com/cloudwu/cstring

2. 3rd/socket-server was copy from https://github.com/cloudwu/socket-server

3. copy from https://github.com/cloudwu/skynet

> * 3rd/lua-md5 

> * lualib-src/lua-clientsocket.c

> * bin.client/client.lua

> * common-src/databuffer.h 

> * common-src/hashid.h

