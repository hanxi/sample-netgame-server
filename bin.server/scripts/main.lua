local socket = require("socket")

HANDSHAKE_CLIENT_MD5 = "3e76b7efa23dbbde535e269023478716"
HANDSHAKE_SERVER_MD5 = "4f74a0ef499702957fa913c1d02f7016"
HANDSHAKE_DEBUG_MD5  = "8d5984dfa10f855f23795857366783fc"

CONFIG_IP = "127.0.0.1"
CONFIG_PORT = 8888

local sock = socket.connect(CONFIG_IP,CONFIG_PORT)

local function send_package(sock,pack)
	local size = #pack
	local package = string.char(bit32.extract(size,8,8)) ..
		string.char(bit32.extract(size,0,8))..
		pack

	print(sock:send(package))
end

local function handshake(sock)
    local protId = 100
    local str = HANDSHAKE_SERVER_MD5
    print(string.format("str=%s",str))
	local sendstr = string.char(bit32.extract(protId,8,8)) ..
		string.char(bit32.extract(protId,0,8))..
		str
    print(string.format("size=%s,str=%s",#sendstr,str))
	send_package(sock,sendstr)
end

handshake(sock)
print(sock)

function msg_dispatch(fd,buffer)
    print("msg_dispatch:",fd,buffer)
    local sock = socket.get_sock(fd)
    print(sock)
end

function disconnect_dispatch(fd)
    print("disconnect_dispatch:",fd)
    socket.delsock(fd)
end

function loop()
    socket.loop()
end

