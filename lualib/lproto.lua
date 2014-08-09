package.cpath="luaclib/?.so;"
local lproto_c = require "lproto.c"

local lproto = {}
local prot = {}
local meta = {
    __index = prot,
}
local BUFFER_MAX_LEN = 10240
local TINTEGER = 1
local TSTRING  = 2

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
function prot:pack(fd,protId,p)
    local protDict = self._dict
    local errstr = string.format("proto is too large. max buffer size : %d",BUFFER_MAX_LEN)
    local sz = 2
    local ret = lproto_c.write_2byte(fd,sz)
    sz = sz + ret
    local ret = lproto_c.write_2byte(protId,sz)
    sz = sz + ret
    for _,key in ipairs(protDict[protId]._keysort) do
        local tp = type(protDict[protId][key])
        if tp=="table" then
            ret = lproto_c.write(#p[key],sz)
            if ret>=BUFFER_MAX_LEN then
                print(errstr)
                return -2
            end
            sz = sz + ret
            for _,v in ipairs(p[key]) do
                for _,k in ipairs(protDict[protId][key]._keysort) do
                    ret = lproto_c.write(v[k],sz)
                    if ret>=BUFFER_MAX_LEN then
                        print(errstr)
                        return -3
                    end
                    sz = sz + ret
                end
            end
        else
            local ret = lproto_c.write(p[key],sz)
            if ret>=BUFFER_MAX_LEN then
                print(errstr)
                return -3
            end
            sz = sz + ret
        end
    end
    local ret = lproto_c.write_2byte(sz-2,0)
    print("buffer size=",sz)
    return lproto_c.newpack(sz)
end

-- return : ret(0), protId, p
function prot:unpack(buffer,sz)
    if sz>=BUFFER_MAX_LEN then
        print(string.format("buffer size limit : %d",BUFFER_MAX_LEN))
        return -1
    end
    lproto_c.unpackinit(buffer)
    local p = {}
    local protDict = self._dict
    local offset = 2
    local ret,fd = lproto_c.read_2byte(offset)
    if ret<0 then
        return -2
    end
    offset = offset + ret
    local ret,protId = lproto_c.read_2byte(offset)
    if ret<0 then
        return -2
    end
    offset = offset + ret
    if not protDict[protId] then
        print(string.format("unknow prot. protId=%d",protId))
    end
    for _,key in ipairs(protDict[protId]._keysort) do
        local tp = type(protDict[protId][key])
        if tp=="table" then
            p[key] = {}
            local ret,n = lproto_c.read(TINTEGER,offset)
            if ret<0 then
                return -3
            end
            offset = offset + ret
            for i=1,n do
                local temp = {}
                for _,k in ipairs(protDict[protId][key]._keysort) do
                    local ttp = type(protDict[protId][key][k])
                    local ittp = TINTEGER
                    if ttp=="string" then ittp = TSTRING end
                    local ret,v = lproto_c.read(ittp,offset)
                    if ret<0 then
                        return -4
                    end
                    offset = offset + ret
                    temp[k] = v 
                end
                table.insert(p[key],temp)
            end
        else
            local itp = TINTEGER
            if tp=="string" then itp = TSTRING end
            local ret,v = lproto_c.read(itp,offset)
            if ret<0 then
                return -5
            end
            offset = offset + ret
            p[key] = v
        end
    end
    return 0,fd,protId,p
end

return lproto

