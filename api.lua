-- api.lua
-- RomM API wrapper for Rommlet

local json = dofile(System.currentDirectory() .. "/json.lua")
local config = dofile(System.currentDirectory() .. "/config.lua")

local api = {}

-- Base64 encoding for Basic Auth
local b64chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'

local function base64_encode(data)
    return ((data:gsub('.', function(x) 
        local r, b = '', x:byte()
        for i = 8, 1, -1 do 
            r = r .. (b % 2^i - b % 2^(i-1) > 0 and '1' or '0') 
        end
        return r
    end) .. '0000'):gsub('%d%d%d?%d?%d?%d?', function(x)
        if #x < 6 then return '' end
        local c = 0
        for i = 1, 6 do 
            c = c + (x:sub(i, i) == '1' and 2^(6-i) or 0) 
        end
        return b64chars:sub(c + 1, c + 1)
    end) .. ({ '', '==', '=' })[#data % 3 + 1])
end

-- Build authorization header value
local function getAuthHeader()
    local user = config.get("username") or ""
    local pass = config.get("password") or ""
    return "Basic " .. base64_encode(user .. ":" .. pass)
end

-- Make HTTP request with Basic Auth using sockets
function api.request(endpoint)
    local serverUrl = config.get("server_url") or ""
    if serverUrl == "" then
        return nil, "Server URL not configured"
    end
    
    -- Parse URL to get host and path
    local protocol, host, port, path = serverUrl:match("^(https?)://([^:/]+):?(%d*)(.*)$")
    if not host then
        host, port, path = serverUrl:match("^([^:/]+):?(%d*)(.*)$")
        protocol = "http"
    end
    
    port = tonumber(port) or 80
    path = path or ""
    if path == "" then path = "/" end
    
    -- Append endpoint to path
    local fullPath = path
    if not fullPath:match("/$") then
        fullPath = fullPath .. "/"
    end
    fullPath = fullPath .. "api" .. endpoint
    
    -- Initialize socket
    Socket.init()
    
    -- Connect to server
    local sock = Socket.connect(host, port)
    if not sock then
        Socket.term()
        return nil, "Failed to connect to server"
    end
    
    -- Build HTTP request
    local authHeader = getAuthHeader()
    local request = "GET " .. fullPath .. " HTTP/1.1\r\n"
    request = request .. "Host: " .. host .. "\r\n"
    request = request .. "Authorization: " .. authHeader .. "\r\n"
    request = request .. "Accept: application/json\r\n"
    request = request .. "Connection: close\r\n"
    request = request .. "\r\n"
    
    -- Send request
    Socket.send(sock, request)
    
    -- Receive response
    local response = ""
    local chunk = Socket.receive(sock, 4096)
    while chunk and #chunk > 0 do
        response = response .. chunk
        chunk = Socket.receive(sock, 4096)
    end
    
    -- Close socket
    Socket.close(sock)
    Socket.term()
    
    -- Parse response
    if response == "" then
        return nil, "Empty response from server"
    end
    
    -- Extract status code
    local statusCode = response:match("HTTP/1%.%d (%d+)")
    statusCode = tonumber(statusCode)
    
    if not statusCode then
        return nil, "Invalid HTTP response"
    end
    
    if statusCode == 401 then
        return nil, "Authentication failed"
    end
    
    if statusCode ~= 200 then
        return nil, "HTTP error: " .. tostring(statusCode)
    end
    
    -- Extract body (after \r\n\r\n)
    local _, bodyStart = response:find("\r\n\r\n")
    if not bodyStart then
        return nil, "Invalid HTTP response format"
    end
    
    local body = response:sub(bodyStart + 1)
    
    -- Handle chunked transfer encoding
    if response:lower():find("transfer%-encoding:%s*chunked") then
        -- Simple chunked decoding
        local decoded = ""
        local pos = 1
        while pos <= #body do
            -- Read chunk size (hex)
            local sizeEnd = body:find("\r\n", pos)
            if not sizeEnd then break end
            local sizeHex = body:sub(pos, sizeEnd - 1)
            local chunkSize = tonumber(sizeHex, 16)
            if not chunkSize or chunkSize == 0 then break end
            
            -- Read chunk data
            local dataStart = sizeEnd + 2
            local dataEnd = dataStart + chunkSize - 1
            decoded = decoded .. body:sub(dataStart, dataEnd)
            
            -- Move past chunk and trailing \r\n
            pos = dataEnd + 3
        end
        body = decoded
    end
    
    -- Parse JSON
    local data = json.decode(body)
    if not data then
        return nil, "Failed to parse JSON response"
    end
    
    return data, nil
end

-- Get all platforms
function api.getPlatforms()
    return api.request("/platforms")
end

-- Get ROMs for a platform (RomM 4.6+ uses platform_ids)
function api.getRoms(platformId, limit, offset)
    limit = limit or 50
    offset = offset or 0
    local endpoint = "/roms?platform_ids=" .. tostring(platformId) 
                  .. "&limit=" .. tostring(limit) 
                  .. "&offset=" .. tostring(offset)
    return api.request(endpoint)
end

-- Get single ROM details
function api.getRom(romId)
    return api.request("/roms/" .. tostring(romId))
end

-- Test connection to server
function api.testConnection()
    local platforms, err = api.getPlatforms()
    if err then
        return false, err
    end
    return true, nil
end

return api
