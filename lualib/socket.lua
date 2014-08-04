local socket_c = require "socket.core"

local sockList = {}
local socket = {}
local _meta = {
    __gc = function (self)
        self:close()
        print("___GC")
    end,
}

function socket.connect (addr,port)
    local sock = {
        _socket = socket_c.connect(addr,port),
        send = function (sock,msg)
            print("socket:send:",sock._socket);
            return socket_c.send(sock._socket,msg)
        end,
        close = function (sock)
            local fd = socket_c.get_fd(sock._socket)
            sockList[fd] = nil
            return socket_c.disconnect(sock._socket)
        end,
        get_fd = function (sock)
            return socket_c.get_fd(sock._socket)
        end,
    }
    print("socket.connect:",sock._socket);
    sock = setmetatable(sock,_meta)
    sockList[sock.get_fd(sock)] = sock
    return sock
end

function socket.loop ()
    for _,sock in pairs(sockList) do
        socket_c.loop(sock._socket)
    end
end

function socket.get_sock(fd)
    return sockList[fd]
end

return socket

