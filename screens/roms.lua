-- screens/roms.lua
-- ROM list screen for Rommlet

local ui = dofile(System.currentDirectory() .. "/ui.lua")
local api = dofile(System.currentDirectory() .. "/api.lua")

local roms = {}

local romList = {}
local selectedIndex = 1
local loading = false
local errorMessage = nil
local platformId = nil
local platformName = ""
local lastPad = 0
local totalRoms = 0
local currentOffset = 0
local pageSize = 50

function roms.init(params)
    params = params or {}
    platformId = params.platformId
    platformName = params.platformName or "ROMs"
    selectedIndex = 1
    romList = {}
    totalRoms = 0
    currentOffset = 0
    errorMessage = nil
    
    if platformId then
        roms.loadPage(0)
    else
        errorMessage = "No platform selected"
    end
end

function roms.loadPage(offset)
    loading = true
    currentOffset = offset
    
    local data, err = api.getRoms(platformId, pageSize, offset)
    loading = false
    
    if err then
        errorMessage = err
        return
    end
    
    -- RomM 4.6+ returns paginated response with items array
    if data then
        -- Handle both array response and paginated response
        local items = data.items or data
        totalRoms = data.total or #items
        
        romList = {}
        if type(items) == "table" then
            for _, rom in ipairs(items) do
                table.insert(romList, {
                    id = rom.id,
                    name = rom.name or rom.fs_name_no_tags or rom.fs_name or "Unknown",
                    fileName = rom.fs_name or "",
                    size = rom.fs_size_bytes or 0
                })
            end
        end
    end
end

function roms.update()
    local pad = Controls.read()
    
    -- Handle input (with debounce)
    if pad ~= lastPad then
        if Controls.check(pad, KEY_DDOWN) then
            selectedIndex = selectedIndex + 1
            if selectedIndex > #romList then
                -- Check if there are more ROMs to load
                if currentOffset + pageSize < totalRoms then
                    roms.loadPage(currentOffset + pageSize)
                    selectedIndex = 1
                else
                    selectedIndex = 1
                end
            end
        elseif Controls.check(pad, KEY_DUP) then
            selectedIndex = selectedIndex - 1
            if selectedIndex < 1 then
                -- Check if we can go to previous page
                if currentOffset > 0 then
                    roms.loadPage(math.max(0, currentOffset - pageSize))
                    selectedIndex = #romList
                else
                    selectedIndex = #romList
                end
            end
        elseif Controls.check(pad, KEY_DRIGHT) then
            -- Next page
            if currentOffset + pageSize < totalRoms then
                roms.loadPage(currentOffset + pageSize)
                selectedIndex = 1
            end
        elseif Controls.check(pad, KEY_DLEFT) then
            -- Previous page
            if currentOffset > 0 then
                roms.loadPage(math.max(0, currentOffset - pageSize))
                selectedIndex = 1
            end
        elseif Controls.check(pad, KEY_A) then
            -- Select ROM (future: show details)
            -- For now, just show the name
        elseif Controls.check(pad, KEY_B) then
            -- Go back to platforms
            return "platforms"
        elseif Controls.check(pad, KEY_Y) then
            -- Refresh current page
            roms.loadPage(currentOffset)
        end
    end
    lastPad = pad
    
    return nil
end

function roms.draw()
    ui.clearAll()
    
    if loading then
        ui.drawLoading("Loading ROMs...")
        Screen.flip()
        return
    end
    
    -- Title with platform name
    local title = platformName
    if #title > 35 then
        title = title:sub(1, 32) .. "..."
    end
    ui.drawTitle(title)
    
    if errorMessage then
        ui.print(4, 30, "Error: " .. errorMessage, ui.colors.error, TOP_SCREEN)
        ui.print(4, 50, "Press B to go back", ui.colors.textDim, TOP_SCREEN)
    elseif #romList == 0 then
        ui.print(4, 30, "No ROMs found", ui.colors.textDim, TOP_SCREEN)
        ui.print(4, 50, "Press B to go back", ui.colors.textDim, TOP_SCREEN)
    else
        -- Draw ROM list
        local y = 24
        local maxVisible = 20
        
        -- Calculate scroll offset within current page
        local scrollOffset = 0
        if selectedIndex > maxVisible then
            scrollOffset = selectedIndex - maxVisible
        end
        
        for i = 1, math.min(maxVisible, #romList - scrollOffset) do
            local idx = i + scrollOffset
            local rom = romList[idx]
            local text = rom.name
            
            -- Truncate name if too long
            local maxNameLen = 46
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
            y = y + ui.LINE_HEIGHT
        end
        
        -- Page/total indicator
        local currentPage = math.floor(currentOffset / pageSize) + 1
        local totalPages = math.ceil(totalRoms / pageSize)
        local indicator = string.format("Page %d/%d (%d ROMs)", currentPage, totalPages, totalRoms)
        ui.print(4, ui.TOP_HEIGHT - 12, indicator, ui.colors.textDim, TOP_SCREEN)
    end
    
    -- Controls on bottom screen
    ui.drawControls({
        { button = "D-Pad U/D", action = "Navigate" },
        { button = "D-Pad L/R", action = "Prev/Next page" },
        { button = "Y", action = "Refresh" },
        { button = "B", action = "Back to platforms" }
    }, BOTTOM_SCREEN)
end

return roms
