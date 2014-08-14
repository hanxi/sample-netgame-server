package.cpath = "luaclib/?.so;"
package.path = "lualib/?.lua;bin.client/?.lua;"

local prot = require("prot")

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
print("protDict:\n"..serialize(prot._dict))

local socket = require "clientsocket"
local bit32 = require "bit32"
local md5 = require "md5"

local fd = assert(socket.connect("127.0.0.1", 8888))

local function unpack_package(text)
    local size = #text
    if size < 2 then
        return nil, text
    end
    local s = text:byte(1) * 256 + text:byte(2)
    if size < s+2 then
        return nil, text
    end

    return text:sub(3,2+s), text:sub(3+s)
end

local function recv_package(last)
    local result
    result, last = unpack_package(last)
    if result then
        return result, last
    end
    local r = socket.recv(fd)
    if not r then
        return nil, last
    end
    if r == "" then
        error "Server closed"
    end
    return unpack_package(last .. r)
end

local session = 0

local function handshake()
    local protId = 100
    local str = md5.sumhexa("client_md5")
    --local str = md5.sumhexa("debug_md5")
    --print(string.format("str=%s",str))
    local sendstr =
        string.char(bit32.extract(fd,8,8))..
        string.char(bit32.extract(fd,0,8))..
        string.char(bit32.extract(protId,8,8)) ..
        string.char(bit32.extract(protId,0,8))..
        str
    local size = #sendstr
    local package =
        string.char(bit32.extract(size,8,8)) ..
        string.char(bit32.extract(size,0,8))..
        sendstr
    socket.sendstring(fd, package)
end

local last = ""

handshake()

while true do
    while true do
        local v
        v, last = recv_package(last)
        if not v then
            break
        end
        local sfd = v:byte(1) * 256 + v:byte(2)
        local result = string.sub(v,3,#v)
        local sz = #v - 2
        local ret,protId,pp = prot:unpack(result,sz)
        print(string.format("recive prot: sz=%d,sfd=%d,protId=%d", sz,sfd,protId))
        print(serialize(pp))
    end
    local cmd = socket.readstdin()
    if cmd then
        local args = {}
        string.gsub(cmd, '[^ ]+', function(v) table.insert(args, v) end )
        -- cmd > protId
        local protId = tonumber(args[1])
        local p = load("return "..table.concat(args," ", 2))()
        local sz,buffer = prot:pack(fd,protId,p)
        print(sz,buffer)
        socket.sendbuffer(fd, buffer,sz)
        socket.usleep(100000)
    end
end
