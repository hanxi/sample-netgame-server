package.cpath = "luaclib/?.so;"
package.path = "lualib/?.lua;"

local socket = require "clientsocket"
local bit32 = require "bit32"
local md5 = require "md5"

local fd = assert(socket.connect("127.0.0.1", 8888))

local function send_package(fd, pack)
	local size = #pack
	local package = string.char(bit32.extract(size,8,8)) ..
		string.char(bit32.extract(size,0,8))..
		pack

	socket.send(fd, package)
end

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

local function send_request(v)
	--session = session + 1
	local str = string.format("%d+%s",session, v)

    local protId = 101
    local mid = -1
	local sendstr = string.char(bit32.extract(protId,8,8)) ..
		string.char(bit32.extract(protId,0,8))..
		string.char(bit32.extract(mid,8,8))..
		string.char(bit32.extract(mid,0,8))..
		str
    print("size=",#sendstr)
	send_package(fd, sendstr)
	print("Request:", sendstr)
end

local function handshake()
    local protId = 100
    --local str = md5.sumhexa("client_md5")
    --print(string.format("str=%s",str))
    --local str = md5.sumhexa("server_md5")
    --print(string.format("str=%s",str))
    local str = md5.sumhexa("debug_md5")
    --print(string.format("str=%s",str))
	local sendstr = string.char(bit32.extract(protId,8,8)) ..
		string.char(bit32.extract(protId,0,8))..
		str
    print(string.format("size=%s,str=%s",#sendstr,str))
	send_package(fd, sendstr)
	print("Request:", sendstr)
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
		local session,t,str = string.match(v, "(%d+)(.)(.*)")
		assert(t == '-' or t == '+')
		session = tonumber(session)
		local result = str
		print("Response:",session, result)
	end
	local cmd = socket.readstdin()
	if cmd then
		local args = {}
		string.gsub(cmd, '[^ ]+', function(v) table.insert(args, v) end )
		send_request(table.concat(args))
	else
		socket.usleep(100)
	end
end
