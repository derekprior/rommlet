-- screens/settings.lua
-- Settings screen for Rommlet

local ui = dofile(System.currentDirectory() .. "/ui.lua")
local config = dofile(System.currentDirectory() .. "/config.lua")

local settings = {}

local fields = {
    { key = "server_url", label = "Server URL", hint = "e.g. http://192.168.1.100" },
    { key = "username", label = "Username", hint = "" },
    { key = "password", label = "Password", hint = "" }
}

local selectedField = 1
local editing = false
local editBuffer = ""
local message = nil
local messageColor = nil

function settings.init()
    selectedField = 1
    editing = false
    editBuffer = ""
    message = nil
end

function settings.update()
    local pad = Controls.read()
    
    if editing then
        -- Handle keyboard input
        local kbState = Keyboard.getState()
        
        if kbState == PRESSED then
            editBuffer = Keyboard.getInput()
        elseif kbState == CLEARED then
            editBuffer = ""
        elseif kbState == FINISHED then
            -- Save the edited value
            config.set(fields[selectedField].key, editBuffer)
            editing = false
            editBuffer = ""
        end
    else
        -- Navigation
        if Controls.check(pad, KEY_DUP) and not Controls.check(Controls.read(), KEY_DUP) then
            -- Wait for release to avoid repeat
        end
        
        if Controls.check(pad, KEY_DDOWN) then
            selectedField = selectedField + 1
            if selectedField > #fields then
                selectedField = 1
            end
        elseif Controls.check(pad, KEY_DUP) then
            selectedField = selectedField - 1
            if selectedField < 1 then
                selectedField = #fields
            end
        elseif Controls.check(pad, KEY_A) then
            -- Start editing current field
            editing = true
            editBuffer = config.get(fields[selectedField].key) or ""
            Keyboard.show()
        elseif Controls.check(pad, KEY_START) then
            -- Save and exit
            if config.save() then
                message = "Settings saved!"
                messageColor = ui.colors.success
                return "platforms"  -- Go to platforms screen
            else
                message = "Failed to save settings"
                messageColor = ui.colors.error
            end
        elseif Controls.check(pad, KEY_B) then
            -- Cancel and go back (if configured)
            if config.isConfigured() then
                return "platforms"
            end
        end
    end
    
    return nil  -- Stay on this screen
end

function settings.draw()
    ui.clearAll()
    
    -- Title
    ui.drawTitle("Rommlet Settings")
    
    -- Fields
    local y = 30
    for i, field in ipairs(fields) do
        local value = config.get(field.key) or ""
        local displayValue = value
        
        -- Mask password
        if field.key == "password" and #value > 0 then
            displayValue = string.rep("*", #value)
        end
        
        -- Show current edit buffer if editing this field
        if editing and i == selectedField then
            if field.key == "password" then
                displayValue = string.rep("*", #editBuffer) .. "_"
            else
                displayValue = editBuffer .. "_"
            end
        end
        
        -- Truncate if too long
        if #displayValue > 40 then
            displayValue = displayValue:sub(1, 37) .. "..."
        end
        
        -- Selection indicator and label
        local labelColor = (i == selectedField) and ui.colors.selected or ui.colors.text
        local prefix = (i == selectedField) and "> " or "  "
        
        ui.print(0, y, prefix .. field.label .. ":", labelColor, TOP_SCREEN)
        y = y + ui.LINE_HEIGHT
        
        -- Value
        local valueColor = (#(config.get(field.key) or "") > 0) and ui.colors.text or ui.colors.textDim
        if editing and i == selectedField then
            valueColor = ui.colors.accent
        end
        ui.print(20, y, displayValue ~= "" and displayValue or "(not set)", valueColor, TOP_SCREEN)
        
        -- Hint
        if field.hint and #field.hint > 0 and i == selectedField and not editing then
            ui.print(20, y + ui.LINE_HEIGHT, field.hint, ui.colors.textDim, TOP_SCREEN)
        end
        
        y = y + ui.LINE_HEIGHT * 2 + 4
    end
    
    -- Message
    if message then
        ui.print(4, ui.TOP_HEIGHT - 30, message, messageColor or ui.colors.text, TOP_SCREEN)
    end
    
    -- Controls on bottom screen
    if editing then
        -- Keyboard is shown
        Keyboard.show()
    else
        ui.drawControls({
            { button = "D-Pad", action = "Navigate fields" },
            { button = "A", action = "Edit field" },
            { button = "START", action = "Save & Continue" },
            { button = "B", action = "Back (if configured)" }
        }, BOTTOM_SCREEN)
    end
end

return settings
