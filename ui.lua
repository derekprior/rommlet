-- ui.lua
-- UI drawing helpers for Rommlet

local ui = {}

-- Colors
ui.colors = {
    background = Color.new(26, 26, 46),      -- Dark blue-gray
    text = Color.new(255, 255, 255),          -- White
    textDim = Color.new(150, 150, 150),       -- Gray
    selected = Color.new(74, 74, 224),        -- Blue highlight
    accent = Color.new(124, 58, 237),         -- Purple
    error = Color.new(239, 68, 68),           -- Red
    success = Color.new(34, 197, 94),         -- Green
    black = Color.new(0, 0, 0)
}

-- Screen dimensions
ui.TOP_WIDTH = 400
ui.TOP_HEIGHT = 240
ui.BOTTOM_WIDTH = 320
ui.BOTTOM_HEIGHT = 240

-- Font metrics (approximate for debug font)
ui.CHAR_WIDTH = 8
ui.CHAR_HEIGHT = 8
ui.LINE_HEIGHT = 10

function ui.clear(screen)
    Screen.clear(screen or TOP_SCREEN)
end

function ui.clearAll()
    Screen.clear(TOP_SCREEN)
    Screen.clear(BOTTOM_SCREEN)
end

function ui.print(x, y, text, color, screen)
    Screen.debugPrint(x, y, text, color or ui.colors.text, screen or TOP_SCREEN)
end

function ui.printCentered(y, text, color, screen)
    local scr = screen or TOP_SCREEN
    local width = (scr == TOP_SCREEN) and ui.TOP_WIDTH or ui.BOTTOM_WIDTH
    local textWidth = #text * ui.CHAR_WIDTH
    local x = math.floor((width - textWidth) / 2)
    Screen.debugPrint(x, y, text, color or ui.colors.text, scr)
end

function ui.drawTitle(text)
    -- Draw title bar background
    ui.print(0, 4, text, ui.colors.accent, TOP_SCREEN)
    -- Divider line position
    local y = 16
    for x = 0, ui.TOP_WIDTH - 1, 4 do
        Screen.debugPrint(x, y, "-", ui.colors.textDim, TOP_SCREEN)
    end
end

function ui.drawList(items, selectedIndex, startY, maxItems, screen)
    local scr = screen or TOP_SCREEN
    local y = startY or 24
    local displayCount = maxItems or 20
    
    -- Calculate scroll offset
    local scrollOffset = 0
    if selectedIndex > displayCount then
        scrollOffset = selectedIndex - displayCount
    end
    
    for i = 1, math.min(displayCount, #items - scrollOffset) do
        local itemIndex = i + scrollOffset
        local item = items[itemIndex]
        local text = item.text or item.name or tostring(item)
        
        -- Truncate if too long
        local maxChars = 48
        if #text > maxChars then
            text = text:sub(1, maxChars - 3) .. "..."
        end
        
        local color = ui.colors.text
        if itemIndex == selectedIndex then
            -- Draw selection indicator
            Screen.debugPrint(0, y, ">", ui.colors.selected, scr)
            color = ui.colors.selected
        end
        
        Screen.debugPrint(12, y, text, color, scr)
        y = y + ui.LINE_HEIGHT
    end
    
    -- Show scroll indicator if needed
    if #items > displayCount then
        local indicator = string.format("[%d/%d]", selectedIndex, #items)
        ui.print(ui.TOP_WIDTH - (#indicator * ui.CHAR_WIDTH) - 4, startY, indicator, ui.colors.textDim, scr)
    end
end

function ui.drawControls(controls, screen)
    local scr = screen or BOTTOM_SCREEN
    local y = 4
    
    ui.print(4, y, "Controls:", ui.colors.accent, scr)
    y = y + ui.LINE_HEIGHT + 4
    
    for _, ctrl in ipairs(controls) do
        local text = ctrl.button .. " - " .. ctrl.action
        ui.print(4, y, text, ui.colors.textDim, scr)
        y = y + ui.LINE_HEIGHT
    end
end

function ui.drawLoading(message)
    ui.clearAll()
    ui.printCentered(ui.TOP_HEIGHT / 2 - 8, message or "Loading...", ui.colors.text, TOP_SCREEN)
end

function ui.drawError(message)
    ui.print(4, ui.TOP_HEIGHT - 20, "Error: " .. (message or "Unknown error"), ui.colors.error, TOP_SCREEN)
end

function ui.drawMessage(message, color)
    ui.printCentered(ui.TOP_HEIGHT / 2 - 8, message, color or ui.colors.text, TOP_SCREEN)
end

return ui
