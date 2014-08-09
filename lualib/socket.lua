local socket_c = require "socket.core"

local sockList_fd2sock = {}
local sockList_name2fd = {}
local sockToDel = {}
local socket = {}
local sock = {}
local meta = {
    __index = sock,
    __gc = function (self)
        print("__GC__")
        self:close()
    end,
    __tostring = function (self)
        return "[sock metatable] : "..self._name
    end
}

function socket.connect (addr,port)
    local name = addr..":"..port
    if sockList_name2fd[name] then
        socket.delsock(sockList_name2fd[name])
    end
    local obj ={
        _socket = socket_c.connect(addr,port),
        _name = name,
    }
    local sock = setmetatable(obj,meta)
    local fd = sock:getfd()
    sockList_fd2sock[fd] = sock
    sockList_name2fd[addr..port] = fd
    return sock
end

function sock:close()
    local fd = self:getfd()
    sockList_fd2sock[fd] = nil
    socket_c.disconnect(self._socket)
end

function sock:disconnect()
    self:close()
    setmetatable(self,nil)
end

function sock:sendbuffer (buffer,sz)
    return socket_c.sendstring(self._socket,buffer,sz)
end

function sock:sendstring (msg)
    return socket_c.sendstring(self._socket,msg)
end

function sock:getfd ()
    return socket_c.getfd(self._socket)
end

function socket.loop ()
    for fd,sock in pairs(sockList_fd2sock) do
        socket_c.loop(sock._socket)
    end
end

function socket.delsock(fd)
    sockList_fd2sock[fd] = nil
end

function socket.getsock(fd)
    return sockList_fd2sock[fd]
end

return socket

