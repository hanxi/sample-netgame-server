local lproto = require("lproto")

local protDict = {}
local startProtId = 101
local protCount = 0
local protIds = {}
local handlers = {}
local tinsert = table.insert
local tsort = table.sort

local function registProt(p,protId_str)
    local protId = startProtId+protCount
    protIds[protId_str] = protId
    protDict[protId] = p
    protCount = protCount + 1
end

local defping = {
    ping = "hello",
}
registProt(defping,"ping_prot_Id")

local defpong = {
    pong = "world",
}
registProt(defpong,"pong_prot_Id")

-- priority 越大优先级越高
function registHandler(protId,handler,priority)
    if not priority or priority<0 then
        priority = 0
    end
    if not handlers[protId] then
        handlers[protId] = {}
    end
    tinsert(handlers[protId],{handler=handler,priority=priority})
    local function cmp(t1,t2)
        return t1.priority>t2.priority
    end
    tsort(handlers[protId],cmp)
end

local function getHandlers(protId)
    return handlers[protId]
end

local prot = lproto.initprot(protDict)
prot.protIds = protIds
prot.getHandlers = getHandlers

return prot

