/*
 * Settings screen - Configure server connection
 */

#include "settings.h"
#include "../ui.h"
#include <stdio.h>
#include <string.h>

typedef enum {
    FIELD_SERVER_URL,
    FIELD_USERNAME,
    FIELD_PASSWORD,
    FIELD_SAVE,
    FIELD_COUNT
} SettingsField;

static Config *currentConfig = NULL;
static int selectedField = FIELD_SERVER_URL;

void settings_init(Config *config) {
    currentConfig = config;
    selectedField = FIELD_SERVER_URL;
}

SettingsResult settings_update(u32 kDown) {
    if (!currentConfig) return SETTINGS_NONE;
    
    // Navigation
    if (kDown & KEY_DOWN) {
        selectedField = (selectedField + 1) % FIELD_COUNT;
    }
    if (kDown & KEY_UP) {
        selectedField = (selectedField - 1 + FIELD_COUNT) % FIELD_COUNT;
    }
    
    // Select/Edit field
    if (kDown & KEY_A) {
        switch (selectedField) {
            case FIELD_SERVER_URL:
                ui_show_keyboard("Server URL", currentConfig->serverUrl, CONFIG_MAX_URL_LEN, false);
                break;
            case FIELD_USERNAME:
                ui_show_keyboard("Username", currentConfig->username, CONFIG_MAX_USER_LEN, false);
                break;
            case FIELD_PASSWORD:
                ui_show_keyboard("Password", currentConfig->password, CONFIG_MAX_PASS_LEN, true);
                break;
            case FIELD_SAVE:
                return SETTINGS_SAVED;
            default:
                break;
        }
    }
    
    // Cancel
    if (kDown & KEY_B) {
        return SETTINGS_CANCELLED;
    }
    
    return SETTINGS_NONE;
}

void settings_draw(void) {
    if (!currentConfig) return;
    
    ui_draw_header("Settings");
    
    float y = UI_HEADER_HEIGHT + UI_PADDING;
    float fieldWidth = SCREEN_TOP_WIDTH - (UI_PADDING * 2);
    
    // Server URL
    ui_draw_text(UI_PADDING, y, "Server URL:", UI_COLOR_TEXT_DIM);
    y += UI_LINE_HEIGHT;
    ui_draw_list_item(UI_PADDING, y, fieldWidth, 
                      currentConfig->serverUrl[0] ? currentConfig->serverUrl : "(not set)",
                      selectedField == FIELD_SERVER_URL);
    y += UI_LINE_HEIGHT + UI_PADDING;
    
    // Username
    ui_draw_text(UI_PADDING, y, "Username:", UI_COLOR_TEXT_DIM);
    y += UI_LINE_HEIGHT;
    ui_draw_list_item(UI_PADDING, y, fieldWidth,
                      currentConfig->username[0] ? currentConfig->username : "(optional)",
                      selectedField == FIELD_USERNAME);
    y += UI_LINE_HEIGHT + UI_PADDING;
    
    // Password
    ui_draw_text(UI_PADDING, y, "Password:", UI_COLOR_TEXT_DIM);
    y += UI_LINE_HEIGHT;
    
    // Mask password
    char masked[64];
    if (currentConfig->password[0]) {
        size_t len = strlen(currentConfig->password);
        if (len > sizeof(masked) - 1) len = sizeof(masked) - 1;
        memset(masked, '*', len);
        masked[len] = '\0';
    } else {
        strcpy(masked, "(optional)");
    }
    ui_draw_list_item(UI_PADDING, y, fieldWidth, masked, selectedField == FIELD_PASSWORD);
    y += UI_LINE_HEIGHT + UI_PADDING * 2;
    
    // Save button
    ui_draw_list_item(UI_PADDING, y, fieldWidth, "[ Save and Connect ]", selectedField == FIELD_SAVE);
    
    // Help text at bottom
    ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING,
                 "D-Pad: Navigate | A: Edit/Select | B: Cancel", UI_COLOR_TEXT_DIM);
}
