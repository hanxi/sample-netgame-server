local socket = require("socket")
local lproto = require("lproto")

local protDict = {}
local function registProt(p)
    table.insert(protDict,p)
    return #protDict
end

local dp1 = {
    ping = "hello",
    ret = -1,
}
local dp2 = {
    pong = "pong",
    list = {
        a = 1,
        b = "hao",
    },
}
local ping_prot_Id = registProt(dp1)
local pong_prot_Id = registProt(dp2)
local prot = lproto.initProt(protDict)

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
    local fd = sock:getfd()
    print(string.format("str=%s",str))
	local sendstr = 
		string.char(bit32.extract(fd,8,8))..
		string.char(bit32.extract(fd,0,8))..
        string.char(bit32.extract(protId,8,8)) ..
		string.char(bit32.extract(protId,0,8))..
		str
    print(string.format("size=%s,str=%s",#sendstr,str))
	send_package(sock,sendstr)
end

handshake(sock)
print(sock)

function msg_dispatch(sfd,cfd,buffer,sz)
    print("msg_dispatch:",sfd,cfd,buffer,sz)
    local sock = socket.getsock(sfd)
    print(sock)
    local ret,protId,pp = prot:unpack(buffer,sz)
    for k,v in pairs(pp or {}) do
        print(k,v)
    end
    if protId == 1 then
        local protId = 2
        local p = {
            pong = "pong",
            list = {
                {
                    a = 2,
                    b = "testlist1",
                },
                {
                    a = 3,
                    b = "testlist2",
                },
            },
        }
        local sz,str = prot:pack(protId,p)
        local sendstr = 
            string.char(bit32.extract(cfd,8,8))..
            string.char(bit32.extract(cfd,0,8))..
            str
        send_package(sock,sendstr)
    end
end

function disconnect_dispatch(fd)
    print("disconnect_dispatch:",fd)
    socket.delsock(fd)
end

function loop()
    socket.loop()
end

