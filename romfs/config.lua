-- config.lua
-- Configuration management for Rommlet

local config = {}

local CONFIG_PATH = "/3ds/rommlet/config.txt"

-- Default configuration
local defaults = {
    server_url = "",
    username = "",
    password = ""
}

-- Current configuration
local current = {}

-- Copy defaults to current
for k, v in pairs(defaults) do
    current[k] = v
end

function config.get(key)
    return current[key]
end

function config.set(key, value)
    current[key] = value
end

function config.getAll()
    return current
end

function config.isConfigured()
    return current.server_url ~= "" and current.username ~= ""
end

function config.load()
    -- Try to open config file
    local file = io.open(CONFIG_PATH, FREAD)
    if not file then
        return false
    end
    
    local size = io.size(file)
    if size == 0 then
        io.close(file)
        return false
    end
    
    local content = io.read(file, 0, size)
    io.close(file)
    
    -- Parse simple key=value format
    for line in content:gmatch("[^\r\n]+") do
        local key, value = line:match("^([^=]+)=(.*)$")
        if key and defaults[key] ~= nil then
            current[key] = value
        end
    end
    
    return true
end

function config.save()
    -- Ensure directory exists (create if needed)
    System.createDirectory("/3ds/rommlet")
    
    -- Build config content
    local content = ""
    content = content .. "server_url=" .. (current.server_url or "") .. "\n"
    content = content .. "username=" .. (current.username or "") .. "\n"
    content = content .. "password=" .. (current.password or "") .. "\n"
    
    -- Write to file
    local file = io.open(CONFIG_PATH, FCREATE)
    if not file then
        return false
    end
    
    io.write(file, 0, content, #content)
    io.close(file)
    return true
end

return config
