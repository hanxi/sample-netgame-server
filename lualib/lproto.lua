package.cpath="luaclib/?.so;"
local lproto_c = require "lproto.c"

local lproto = {}
local prot = {}
local meta = {
    __index = prot,
}
local BUFFER_MAX_LEN = 10240

--[[ buffer content
-- | - 2byte protId - | - prot - |
--]]

local function pairsbykey(t,f)
    if t == nil then
        return
    end
    local a = {}
    for n in pairs(t) do table.insert(a, n) end
    table.sort(a, f)
    local i = 0                 -- iterator variable
    local iter = function ()    -- iterator function
       i = i + 1
       if a[i] == nil then return nil
       else return a[i], t[a[i]]
       end
    end
    return iter
end

function lproto.initProt(protDict)
    for protId,p in pairs(protDict) do
        local _keysort = {}
        for k,v in pairsbykey(p) do
            if type(v)=="table" then
                local __keysort = {}
                for kk,vv in pairsbykey(v) do
                    if kk~="_keysort" then
                        table.insert(__keysort,kk)
                    end
                end
                v._keysort = __keysort
            end
            if kk~="_keysort" then
                table.insert(_keysort,k)
            end
        end
        protDict[protId]._keysort = _keysort
    end
    local obj = {
        _dict = protDict,
    }
    return setmetatable(obj,meta)
end

-- return : sz,buffer
function prot:pack(protId,p)
    local protDict = self._dict
    local errstr = string.format("proto is too large. max buffer size : %d",BUFFER_MAX_LEN)
    lproto_c.packinit()
    local ret = lproto_c.writeid(protId)
    for _,key in ipairs(protDict[protId]._keysort) do
        local tp = type(protDict[protId][key])
        if tp=="table" then
            local ret = lproto_c.write(#p[key])
            if ret>=BUFFER_MAX_LEN then
                print(errstr)
                return -1
            end
            for _,v in ipairs(p[key]) do
                for _,k in ipairs(protDict[protId][key]._keysort) do
                    local ret = lproto_c.write(v[k])
                    if ret>=BUFFER_MAX_LEN then
                        print(errstr)
                        return -1
                    end
                end
            end
        else
            local ret = lproto_c.write(p[key])
            if ret>=BUFFER_MAX_LEN then
                print(errstr)
                return -3
            end
        end
    end
    return lproto_c.getpack()
end

-- return : ret(0), protId, p
function prot:unpack(buffer,sz)
    if sz>=BUFFER_MAX_LEN then
        print(string.format("buffer size limit : %d",BUFFER_MAX_LEN))
        return -1
    end
    lproto_c.unpackinit(sz,buffer)
    local p = {}
    local protDict = self._dict
    local ret,protId = lproto_c.readid()
    if ret<0 then
        return -2
    end
    if not protDict[protId] then
        print(string.format("unknow prot. protId=%d",protId))
    end
    for _,key in ipairs(protDict[protId]._keysort) do
        local tp = type(protDict[protId][key])
        if tp=="table" then
            p[key] = {}
            local ret,n = lproto_c.read("number")
            if ret<0 then
                return -3
            end
            for i=1,n do
                local temp = {}
                for _,k in ipairs(protDict[protId][key]._keysort) do
                    local ttp = type(protDict[protId][key][k])
                    local ret,v = lproto_c.read(ttp)
                    if ret<0 then
                        return -4
                    end
                    temp[k] = v 
                end
                table.insert(p[key],temp)
            end
        else
            local ret,v = lproto_c.read(tp)
            if ret<0 then
                return -5
            end
            p[key] = v
        end
    end
    return 0,protId,p
end

return lproto

