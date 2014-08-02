local socket = require("socket")

HANDSHAKE_CLIENT_MD5 = "3e76b7efa23dbbde535e269023478716"
HANDSHAKE_SERVER_MD5 = "4f74a0ef499702957fa913c1d02f7016"
HANDSHAKE_DEBUG_MD5  = "8d5984dfa10f855f23795857366783fc"

CONFIG_IP = "127.0.0.1"
CONFIG_PORT = 8888

socket.connect(CONFIG_IP,CONFIG_PORT)

local function send_package(pack)
	local size = #pack
	local package = string.char(bit32.extract(size,8,8)) ..
		string.char(bit32.extract(size,0,8))..
		pack

	socket.send(package)
end

local function handshake()
    local protId = 100
    local str = HANDSHAKE_SERVER_MD5
    print(string.format("str=%s",str))
	local sendstr = string.char(bit32.extract(protId,8,8)) ..
		string.char(bit32.extract(protId,0,8))..
		str
    print(string.format("size=%s,str=%s",#sendstr,str))
	send_package(sendstr)
end
handshake()

function msg_dispatch(id,buffer)
    print("msg_dispatch:",id,buffer)
end

function loop()
--    print("lua loop")
end

