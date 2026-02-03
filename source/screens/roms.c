/*
 * ROMs screen - Display list of ROMs for a platform
 */

#include "roms.h"
#include "../ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOAD_MORE_THRESHOLD 5

static Rom *romList = NULL;
static int romCount = 0;
static int romTotal = 0;
static char currentPlatform[128] = "";
static int selectedIndex = 0;
static int scrollOffset = 0;

void roms_init(void) {
    romList = NULL;
    romCount = 0;
    romTotal = 0;
    currentPlatform[0] = '\0';
    selectedIndex = 0;
    scrollOffset = 0;
}

void roms_set_data(Rom *roms, int count, int total, const char *platformName) {
    romList = roms;
    romCount = count;
    romTotal = total;
    strncpy(currentPlatform, platformName, sizeof(currentPlatform) - 1);
    selectedIndex = 0;
    scrollOffset = 0;
}

void roms_append_data(Rom *roms, int count) {
    if (!roms || count == 0) return;
    
    Rom *newList = realloc(romList, (romCount + count) * sizeof(Rom));
    if (!newList) {
        free(roms);
        return;
    }
    
    romList = newList;
    memcpy(&romList[romCount], roms, count * sizeof(Rom));
    romCount += count;
    free(roms);
}

bool roms_needs_more_data(void) {
    return romCount < romTotal && selectedIndex >= romCount - LOAD_MORE_THRESHOLD;
}

int roms_get_count(void) {
    return romCount;
}

RomsResult roms_update(u32 kDown, int *outSelectedIndex) {
    // Back to platforms
    if (kDown & KEY_B) {
        return ROMS_BACK;
    }
    
    if (!romList || romCount == 0) {
        return ROMS_NONE;
    }
    
    // Navigation
    if (kDown & KEY_DOWN) {
        selectedIndex++;
        if (selectedIndex >= romCount) {
            selectedIndex = 0;
            scrollOffset = 0;
        }
        if (selectedIndex >= scrollOffset + UI_VISIBLE_ITEMS) {
            scrollOffset = selectedIndex - UI_VISIBLE_ITEMS + 1;
        }
    }
    
    if (kDown & KEY_UP) {
        selectedIndex--;
        if (selectedIndex < 0) {
            selectedIndex = romCount - 1;
            scrollOffset = romCount > UI_VISIBLE_ITEMS ? romCount - UI_VISIBLE_ITEMS : 0;
        }
        if (selectedIndex < scrollOffset) {
            scrollOffset = selectedIndex;
        }
    }
    
    // Page up/down
    if (kDown & KEY_R) {
        selectedIndex += UI_VISIBLE_ITEMS;
        if (selectedIndex >= romCount) {
            selectedIndex = romCount - 1;
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
    
    // Select ROM
    if (kDown & KEY_A) {
        if (outSelectedIndex) *outSelectedIndex = selectedIndex;
        return ROMS_SELECTED;
    }
    
    // Check if we need to load more data
    if (roms_needs_more_data()) {
        return ROMS_LOAD_MORE;
    }
    
    return ROMS_NONE;
}

void roms_draw(void) {
    // Header with platform name
    char headerText[192];
    snprintf(headerText, sizeof(headerText), "ROMs - %s", currentPlatform);
    ui_draw_header(headerText);
    
    if (!romList || romCount == 0) {
        ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT / 2, 
                     "No ROMs found for this platform.", UI_COLOR_TEXT_DIM);
        ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING,
                     "B: Back to Platforms", UI_COLOR_TEXT_DIM);
        return;
    }
    
    float y = UI_HEADER_HEIGHT + UI_PADDING;
    float itemWidth = SCREEN_TOP_WIDTH - (UI_PADDING * 2);
    
    // Draw visible items
    int visibleEnd = scrollOffset + UI_VISIBLE_ITEMS;
    if (visibleEnd > romCount) visibleEnd = romCount;
    
    for (int i = scrollOffset; i < visibleEnd; i++) {
        ui_draw_list_item(UI_PADDING, y, itemWidth, romList[i].name, i == selectedIndex);
        y += UI_LINE_HEIGHT;
    }
    
    // Scroll/count indicator
    char scrollText[64];
    snprintf(scrollText, sizeof(scrollText), "%d/%d (showing %d)", 
             selectedIndex + 1, romTotal, romCount);
    float textWidth = ui_get_text_width(scrollText);
    ui_draw_text(SCREEN_TOP_WIDTH - textWidth - UI_PADDING, 
                 UI_HEADER_HEIGHT + UI_PADDING, scrollText, UI_COLOR_TEXT_DIM);
    
    // Help text
    ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING,
                 "A: Details | L/R: Page | B: Back", UI_COLOR_TEXT_DIM);
}
