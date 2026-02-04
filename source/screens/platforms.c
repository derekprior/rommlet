/*
 * Platforms screen - Display list of platforms from RomM
 */

#include "platforms.h"
#include "../ui.h"
#include <stdio.h>
#include <string.h>

static Platform *platformList = NULL;
static int platformCount = 0;
static int selectedIndex = 0;
static int scrollOffset = 0;

void platforms_init(void) {
    platformList = NULL;
    platformCount = 0;
    selectedIndex = 0;
    scrollOffset = 0;
}

void platforms_set_data(Platform *platforms, int count) {
    platformList = platforms;
    platformCount = count;
    selectedIndex = 0;
    scrollOffset = 0;
}

PlatformsResult platforms_update(u32 kDown, int *outSelectedIndex) {
    if (!platformList || platformCount == 0) {
        // No platforms - allow refresh
        if (kDown & KEY_X) {
            return PLATFORMS_REFRESH;
        }
        return PLATFORMS_NONE;
    }
    
    // Navigation
    if (kDown & KEY_DOWN) {
        selectedIndex++;
        if (selectedIndex >= platformCount) {
            selectedIndex = 0;
            scrollOffset = 0;
        }
        // Scroll if needed
        if (selectedIndex >= scrollOffset + UI_VISIBLE_ITEMS) {
            scrollOffset = selectedIndex - UI_VISIBLE_ITEMS + 1;
        }
    }
    
    if (kDown & KEY_UP) {
        selectedIndex--;
        if (selectedIndex < 0) {
            selectedIndex = platformCount - 1;
            scrollOffset = platformCount > UI_VISIBLE_ITEMS ? platformCount - UI_VISIBLE_ITEMS : 0;
        }
        // Scroll if needed
        if (selectedIndex < scrollOffset) {
            scrollOffset = selectedIndex;
        }
    }
    
    // Page up/down
    if (kDown & KEY_R) {
        selectedIndex += UI_VISIBLE_ITEMS;
        if (selectedIndex >= platformCount) {
            selectedIndex = platformCount - 1;
        }
        if (selectedIndex >= scrollOffset + UI_VISIBLE_ITEMS) {
            scrollOffset = selectedIndex - UI_VISIBLE_ITEMS + 1;
        }
    }
    
    if (kDown & KEY_L) {
        selectedIndex -= UI_VISIBLE_ITEMS;
        if (selectedIndex < 0) {
            selectedIndex = 0;
        }
        if (selectedIndex < scrollOffset) {
            scrollOffset = selectedIndex;
        }
    }
    
    // Select platform
    if (kDown & KEY_A) {
        if (outSelectedIndex) *outSelectedIndex = selectedIndex;
        return PLATFORMS_SELECTED;
    }
    
    // Refresh
    if (kDown & KEY_X) {
        return PLATFORMS_REFRESH;
    }
    
    return PLATFORMS_NONE;
}

void platforms_draw(void) {
    ui_draw_header("Platforms");
    
    if (!platformList || platformCount == 0) {
        ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT / 2, 
                     "No platforms found. Press X to refresh.", UI_COLOR_TEXT_DIM);
        ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING,
                     "X: Refresh", UI_COLOR_TEXT_DIM);
        return;
    }
    
    float y = UI_HEADER_HEIGHT + UI_PADDING;
    float itemWidth = SCREEN_TOP_WIDTH - (UI_PADDING * 2);
    
    // Draw visible items
    int visibleEnd = scrollOffset + UI_VISIBLE_ITEMS;
    if (visibleEnd > platformCount) visibleEnd = platformCount;
    
    for (int i = scrollOffset; i < visibleEnd; i++) {
        char itemText[192];
        snprintf(itemText, sizeof(itemText), "%s (%d ROMs)", 
                 platformList[i].displayName, platformList[i].romCount);
        
        ui_draw_list_item(UI_PADDING, y, itemWidth, itemText, i == selectedIndex);
        y += UI_LINE_HEIGHT;
    }
    
    // Scroll indicator
    if (platformCount > UI_VISIBLE_ITEMS) {
        char scrollText[32];
        snprintf(scrollText, sizeof(scrollText), "%d/%d", selectedIndex + 1, platformCount);
        float textWidth = ui_get_text_width(scrollText);
        ui_draw_text(SCREEN_TOP_WIDTH - textWidth - UI_PADDING, 
                     UI_HEADER_HEIGHT + UI_PADDING, scrollText, UI_COLOR_TEXT_DIM);
    }
    
    // Help text
    ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING,
                 "A: Select | X: Refresh", UI_COLOR_TEXT_DIM);
}
