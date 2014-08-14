
local function sendprot(sock,cfd,protId,p)
    local sz,buf = prot:pack(cfd,protId,p)
    sock:sendbuffer(buf,sz)
end

local function pinghandler (sock,cfd,recvprot)
    local protId = prot.protIds.pong_prot_Id
    local p = {
        pong = "pong",
    }
    sendprot(sock,cfd,protId,p)
end
registHandler(prot.protIds.ping_prot_Id,pinghandler)
