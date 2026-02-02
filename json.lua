-- json.lua
-- Simple JSON parser for lpp-3ds
-- Only handles the subset of JSON we need from RomM API

local json = {}

local function skip_whitespace(str, pos)
    while pos <= #str do
        local c = str:sub(pos, pos)
        if c == " " or c == "\t" or c == "\n" or c == "\r" then
            pos = pos + 1
        else
            break
        end
    end
    return pos
end

local function parse_string(str, pos)
    -- Expect opening quote
    if str:sub(pos, pos) ~= '"' then
        return nil, pos
    end
    pos = pos + 1
    
    local result = ""
    while pos <= #str do
        local c = str:sub(pos, pos)
        if c == '"' then
            return result, pos + 1
        elseif c == '\\' then
            pos = pos + 1
            local escape = str:sub(pos, pos)
            if escape == 'n' then result = result .. "\n"
            elseif escape == 'r' then result = result .. "\r"
            elseif escape == 't' then result = result .. "\t"
            elseif escape == '"' then result = result .. '"'
            elseif escape == '\\' then result = result .. '\\'
            elseif escape == '/' then result = result .. '/'
            elseif escape == 'u' then
                -- Skip unicode escapes, just put a placeholder
                pos = pos + 4
                result = result .. "?"
            else
                result = result .. escape
            end
            pos = pos + 1
        else
            result = result .. c
            pos = pos + 1
        end
    end
    return result, pos
end

local function parse_number(str, pos)
    local start = pos
    local c = str:sub(pos, pos)
    
    -- Optional minus
    if c == '-' then
        pos = pos + 1
    end
    
    -- Digits
    while pos <= #str do
        c = str:sub(pos, pos)
        if c:match("[0-9%.eE%+%-]") then
            pos = pos + 1
        else
            break
        end
    end
    
    local num_str = str:sub(start, pos - 1)
    return tonumber(num_str), pos
end

local parse_value  -- Forward declaration

local function parse_array(str, pos)
    -- Expect opening bracket
    if str:sub(pos, pos) ~= '[' then
        return nil, pos
    end
    pos = pos + 1
    
    local result = {}
    pos = skip_whitespace(str, pos)
    
    -- Empty array
    if str:sub(pos, pos) == ']' then
        return result, pos + 1
    end
    
    while pos <= #str do
        local value
        value, pos = parse_value(str, pos)
        table.insert(result, value)
        
        pos = skip_whitespace(str, pos)
        local c = str:sub(pos, pos)
        
        if c == ']' then
            return result, pos + 1
        elseif c == ',' then
            pos = pos + 1
            pos = skip_whitespace(str, pos)
        else
            break
        end
    end
    
    return result, pos
end

local function parse_object(str, pos)
    -- Expect opening brace
    if str:sub(pos, pos) ~= '{' then
        return nil, pos
    end
    pos = pos + 1
    
    local result = {}
    pos = skip_whitespace(str, pos)
    
    -- Empty object
    if str:sub(pos, pos) == '}' then
        return result, pos + 1
    end
    
    while pos <= #str do
        -- Parse key
        pos = skip_whitespace(str, pos)
        local key
        key, pos = parse_string(str, pos)
        if not key then break end
        
        -- Expect colon
        pos = skip_whitespace(str, pos)
        if str:sub(pos, pos) ~= ':' then break end
        pos = pos + 1
        
        -- Parse value
        pos = skip_whitespace(str, pos)
        local value
        value, pos = parse_value(str, pos)
        result[key] = value
        
        pos = skip_whitespace(str, pos)
        local c = str:sub(pos, pos)
        
        if c == '}' then
            return result, pos + 1
        elseif c == ',' then
            pos = pos + 1
        else
            break
        end
    end
    
    return result, pos
end

parse_value = function(str, pos)
    pos = skip_whitespace(str, pos)
    local c = str:sub(pos, pos)
    
    if c == '"' then
        return parse_string(str, pos)
    elseif c == '[' then
        return parse_array(str, pos)
    elseif c == '{' then
        return parse_object(str, pos)
    elseif c == 't' then
        -- true
        if str:sub(pos, pos + 3) == "true" then
            return true, pos + 4
        end
    elseif c == 'f' then
        -- false
        if str:sub(pos, pos + 4) == "false" then
            return false, pos + 5
        end
    elseif c == 'n' then
        -- null
        if str:sub(pos, pos + 3) == "null" then
            return nil, pos + 4
        end
    elseif c == '-' or c:match("[0-9]") then
        return parse_number(str, pos)
    end
    
    return nil, pos
end

function json.decode(str)
    if not str or str == "" then
        return nil
    end
    local result, _ = parse_value(str, 1)
    return result
end

return json
