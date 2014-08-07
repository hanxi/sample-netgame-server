
## simple network game server

> ### gateway

> ### logic server

> ### console client

> ### net protocal (lproto)


## test in linux

* compile and run


    $ make                          # compile
    $ ./bin.gateway/gateway         # start gateway (on tty 1)
    $ ./bin.server/server           # start server (on other tty2)
    $ lua ./bin.client/client.lua   # start client (on other tty3)
    $ 1 {ping="xxx",ret=2,}         # send request to server (on client tty3)


### about test

1. start gateway, wait message from client or server

2. start server, connect to gateway, gateway set server and save server fd

3. start client, connect to gateway and connect server, send others message to server

4. input cmd in client tty: 1 {ping="xxx",ret=2,} . will send protocal to server. cmd like : protId {} (prot must define)

5. if you want connect to gateway as debugclient, change client handshake md5 string for "debug_md5" in client.lua


## others :

1. 3rd/cstring was copy from https://github.com/cloudwu/cstring

2. 3rd/socket-server was copy from https://github.com/cloudwu/socket-server

3. copy from https://github.com/cloudwu/skynet

> * 3rd/lua-md5 

> * lualib-src/lua-clientsocket.c

> * bin.client/client.lua

> * common-src/databuffer.h 

> * common-src/hashid.h

