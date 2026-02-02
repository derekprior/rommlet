-- index.lua
-- Rommlet - RomM Client for Nintendo 3DS
-- Main entry point and state machine

-- Load modules
local config = dofile(System.currentDirectory() .. "/config.lua")
local ui = dofile(System.currentDirectory() .. "/ui.lua")

-- Screen modules (loaded on demand)
local screens = {}
local currentScreen = nil
local screenParams = nil

-- Load a screen module
local function loadScreen(name)
    if not screens[name] then
        screens[name] = dofile(System.currentDirectory() .. "/screens/" .. name .. ".lua")
    end
    return screens[name]
end

-- Switch to a new screen
local function switchScreen(name, params)
    currentScreen = name
    screenParams = params
    local screen = loadScreen(name)
    if screen and screen.init then
        screen.init(params)
    end
end

-- Initialize app
local function init()
    -- Load configuration
    config.load()
    
    -- Determine starting screen
    if config.isConfigured() then
        switchScreen("platforms")
    else
        switchScreen("settings")
    end
end

-- Main loop
local function main()
    init()
    
    while true do
        -- Wait for vblank
        Screen.waitVblankStart()
        Screen.refresh()
        
        -- Get current screen module
        local screen = loadScreen(currentScreen)
        
        -- Update current screen
        if screen and screen.update then
            local nextScreen, params = screen.update()
            if nextScreen then
                switchScreen(nextScreen, params)
                screen = loadScreen(currentScreen)
            end
        end
        
        -- Draw current screen
        if screen and screen.draw then
            screen.draw()
        end
        
        -- Flip screens
        Screen.flip()
    end
end

-- Run the app
main()
