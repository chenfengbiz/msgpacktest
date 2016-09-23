local mp = require 'MessagePack'
local testpkg = require 'testpkg'

local function testmethod(arg1, arg2)
    return arg1..arg2
end

local function onGameData(id, data)
    local args = mp.unpack(data)
    local method=args[1]
    table.remove(args, 1)
    return testpkg.methods[method](unpack(args))
end

local function onGameDataEx(id, ...)
    local args = {...}
    local method = args[1]
    table.remove(args, 1)
    return testpkg.methods[method](unpack(args))
end

testpkg.methods = {}
testpkg.methods[1] = testmethod
testpkg.onGameDataEx = onGameDataEx
testpkg.onGameData = onGameData





