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

    sock:send(package)
end

local function handshake(sock)
    local protId = 100
    local str = HANDSHAKE_SERVER_MD5
    local fd = sock:getfd()
    print(string.format("server md5 : %s",str))
    local sendstr =
        string.char(bit32.extract(fd,8,8))..
        string.char(bit32.extract(fd,0,8))..
        string.char(bit32.extract(protId,8,8)) ..
        string.char(bit32.extract(protId,0,8))..
        str

    local size = #sendstr
    local package = string.char(bit32.extract(size,8,8)) ..
        string.char(bit32.extract(size,0,8))..
        sendstr
    print(string.format("[handshake] size=%s,str=%s",size,str))
    sock:sendstring(package)
end

handshake(sock)
print(sock)

-- 序列化
function serialize(obj,n)
    if n==nil then n=1 end
    local lua = ""
    local t = type(obj)
    if t == "number" then
        lua = lua .. obj
    elseif t == "boolean" then
        lua = lua .. tostring(obj)
    elseif t == "string" then
        lua = lua .. string.format("%q", obj)
    elseif t == "table" then
        lua = lua .. "{\n"
        for k, v in pairs(obj) do
            lua = lua .. string.rep("\t",n) .. "[" .. serialize(k) .. "]=" .. serialize(v,n+1) .. ",\n"
        end
        local metatable = getmetatable(obj)
        if metatable ~= nil and type(metatable.__index) == "table" then
            for k, v in pairs(metatable.__index) do
                lua = lua .. string.rep("\t",n) .."[" .. serialize(k) .. "]=" .. serialize(v,n+1) .. ",\n"
            end
        end
        lua = lua .. string.rep("\t",n-1) .. "}"
    elseif t == "nil" then
        return nil
    else
        error("can not serialize a " .. t .. " type.")
    end
    return lua
end

function msg_dispatch(sfd,buffer,sz)
    local sock = socket.getsock(sfd)
    local ret,cfd,protId,pp = prot:unpack(buffer,sz)
    print(string.format("[msg_dispatch] sfd=%d,cfd=%d,protId=%d,sz=%d,sock=%s",sfd,cfd,protId,sz,sock))
    print("recive prot :\n"..serialize(pp))
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
        local sz,buf = prot:pack(cfd,protId,p)
        sock:sendbuffer(buf,sz)
    end
end

function disconnect_dispatch(fd)
    print("[disconnect_dispatch] ",fd)
    socket.delsock(fd)
end

function loop()
    socket.loop()
end

