-- screens/platforms.lua
-- Platforms list screen for Rommlet

local ui = dofile(System.currentDirectory() .. "/ui.lua")
local api = dofile(System.currentDirectory() .. "/api.lua")

local platforms = {}

local platformList = {}
local selectedIndex = 1
local loading = false
local errorMessage = nil
local lastPad = 0

function platforms.init()
    selectedIndex = 1
    loading = true
    errorMessage = nil
    platformList = {}
    
    -- Fetch platforms from API
    local data, err = api.getPlatforms()
    loading = false
    
    if err then
        errorMessage = err
        return
    end
    
    if data and type(data) == "table" then
        for _, platform in ipairs(data) do
            table.insert(platformList, {
                id = platform.id,
                name = platform.display_name or platform.custom_name or platform.name or "Unknown",
                romCount = platform.rom_count or 0,
                slug = platform.slug
            })
        end
        
        -- Sort by name
        table.sort(platformList, function(a, b)
            return a.name:lower() < b.name:lower()
        end)
    end
end

function platforms.refresh()
    platforms.init()
end

function platforms.update()
    local pad = Controls.read()
    
    -- Handle input (with debounce)
    if pad ~= lastPad then
        if Controls.check(pad, KEY_DDOWN) then
            selectedIndex = selectedIndex + 1
            if selectedIndex > #platformList then
                selectedIndex = 1
            end
        elseif Controls.check(pad, KEY_DUP) then
            selectedIndex = selectedIndex - 1
            if selectedIndex < 1 then
                selectedIndex = #platformList
            end
        elseif Controls.check(pad, KEY_A) then
            -- Select platform, go to ROMs
            if #platformList > 0 then
                local selected = platformList[selectedIndex]
                return "roms", { platformId = selected.id, platformName = selected.name }
            end
        elseif Controls.check(pad, KEY_Y) then
            -- Refresh
            platforms.refresh()
        elseif Controls.check(pad, KEY_SELECT) then
            -- Go to settings
            return "settings"
        elseif Controls.check(pad, KEY_START) then
            -- Exit app
            System.exit()
        end
    end
    lastPad = pad
    
    return nil
end

function platforms.draw()
    ui.clearAll()
    
    if loading then
        ui.drawLoading("Loading platforms...")
        Screen.flip()
        return
    end
    
    -- Title
    ui.drawTitle("Rommlet - Platforms")
    
    if errorMessage then
        ui.print(4, 30, "Error: " .. errorMessage, ui.colors.error, TOP_SCREEN)
        ui.print(4, 50, "Press SELECT to check settings", ui.colors.textDim, TOP_SCREEN)
    elseif #platformList == 0 then
        ui.print(4, 30, "No platforms found", ui.colors.textDim, TOP_SCREEN)
        ui.print(4, 50, "Press Y to refresh", ui.colors.textDim, TOP_SCREEN)
    else
        -- Draw platform list
        local y = 24
        local maxVisible = 20
        
        -- Calculate scroll offset
        local scrollOffset = 0
        if selectedIndex > maxVisible then
            scrollOffset = selectedIndex - maxVisible
        end
        
        for i = 1, math.min(maxVisible, #platformList - scrollOffset) do
            local idx = i + scrollOffset
            local platform = platformList[idx]
            local text = platform.name
            local countText = "(" .. tostring(platform.romCount) .. ")"
            
            -- Truncate name if too long
            local maxNameLen = 40
            if #text > maxNameLen then
                text = text:sub(1, maxNameLen - 3) .. "..."
            end
            
            local color = ui.colors.text
            local prefix = "  "
            if idx == selectedIndex then
                prefix = "> "
                color = ui.colors.selected
            end
            
            ui.print(0, y, prefix .. text, color, TOP_SCREEN)
            ui.print(ui.TOP_WIDTH - (#countText * ui.CHAR_WIDTH) - 8, y, countText, ui.colors.textDim, TOP_SCREEN)
            
            y = y + ui.LINE_HEIGHT
        end
        
        -- Scroll indicator
        if #platformList > maxVisible then
            local indicator = string.format("[%d/%d]", selectedIndex, #platformList)
            ui.print(4, ui.TOP_HEIGHT - 12, indicator, ui.colors.textDim, TOP_SCREEN)
        end
    end
    
    -- Controls on bottom screen
    ui.drawControls({
        { button = "D-Pad", action = "Navigate" },
        { button = "A", action = "Select platform" },
        { button = "Y", action = "Refresh" },
        { button = "SELECT", action = "Settings" },
        { button = "START", action = "Exit" }
    }, BOTTOM_SCREEN)
end

function platforms.getSelectedPlatform()
    if #platformList > 0 and selectedIndex <= #platformList then
        return platformList[selectedIndex]
    end
    return nil
end

return platforms
