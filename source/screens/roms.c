/*
 * ROMs screen - Display list of ROMs for a platform
 */

#include "roms.h"
#include "../ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Rom *romList = NULL;
static int romCount = 0;
static int romTotal = 0;
static char currentPlatform[128] = "";
static int selectedIndex = 0;
static int scrollOffset = 0;

// Check if there are more ROMs to load
static bool has_more_to_load(void) {
    return romCount < romTotal;
}

// Get total display count (including "Load more..." row if applicable)
static int get_display_count(void) {
    return romCount + (has_more_to_load() ? 1 : 0);
}

void roms_init(void) {
    romList = NULL;
    romCount = 0;
    romTotal = 0;
    currentPlatform[0] = '\0';
    selectedIndex = 0;
    scrollOffset = 0;
}

void roms_clear(void) {
    if (romList) {
        free(romList);
        romList = NULL;
    }
    romCount = 0;
    romTotal = 0;
    currentPlatform[0] = '\0';
    selectedIndex = 0;
    scrollOffset = 0;
}

void roms_set_data(Rom *roms, int count, int total, const char *platformName) {
    // Free any existing data first
    if (romList) {
        free(romList);
    }
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
    // Now triggered when "Load more..." row is selected
    return has_more_to_load() && selectedIndex == romCount;
}

int roms_get_count(void) {
    return romCount;
}

int roms_get_id_at(int index) {
    if (!romList || index < 0 || index >= romCount) {
        return -1;
    }
    return romList[index].id;
}

const Rom *roms_get_at(int index) {
    if (!romList || index < 0 || index >= romCount) {
        return NULL;
    }
    return &romList[index];
}

int roms_get_selected_index(void) {
    return selectedIndex;
}

RomsResult roms_update(u32 kDown, int *outSelectedIndex) {
    // Back to platforms
    if (kDown & KEY_B) {
        return ROMS_BACK;
    }
    
    if (!romList || romCount == 0) {
        return ROMS_NONE;
    }
    
    int displayCount = get_display_count();
    
    // Navigation
    if (kDown & KEY_DOWN) {
        selectedIndex++;
        if (selectedIndex >= displayCount) {
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
            selectedIndex = displayCount - 1;
            scrollOffset = displayCount > UI_VISIBLE_ITEMS ? displayCount - UI_VISIBLE_ITEMS : 0;
        }
        if (selectedIndex < scrollOffset) {
            scrollOffset = selectedIndex;
        }
    }
    
    // Page up/down
    if (kDown & KEY_R) {
        selectedIndex += UI_VISIBLE_ITEMS;
        if (selectedIndex >= displayCount) {
            selectedIndex = displayCount - 1;
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
    
    // Select ROM (only if not on "Load more..." row)
    if (kDown & KEY_A) {
        if (selectedIndex < romCount) {
            if (outSelectedIndex) *outSelectedIndex = selectedIndex;
            return ROMS_SELECTED;
        }
    }
    
    // Check if we're on "Load more..." row
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
    
    int displayCount = get_display_count();
    
    // Draw visible items
    int visibleEnd = scrollOffset + UI_VISIBLE_ITEMS;
    if (visibleEnd > displayCount) visibleEnd = displayCount;
    
    for (int i = scrollOffset; i < visibleEnd; i++) {
        if (i < romCount) {
            ui_draw_list_item(UI_PADDING, y, itemWidth, romList[i].name, i == selectedIndex);
        } else {
            // "Load more..." row
            if (i == selectedIndex) {
                ui_draw_rect(UI_PADDING, y, itemWidth, UI_LINE_HEIGHT, UI_COLOR_SELECTED);
                ui_draw_text(UI_PADDING + UI_PADDING, y + 2, "Load more...", UI_COLOR_TEXT);
            } else {
                ui_draw_text(UI_PADDING + UI_PADDING, y + 2, "Load more...", UI_COLOR_TEXT_DIM);
            }
        }
        y += UI_LINE_HEIGHT;
    }
    
    // Scroll/count indicator
    char scrollText[64];
    int displayIndex = selectedIndex < romCount ? selectedIndex + 1 : romCount;
    snprintf(scrollText, sizeof(scrollText), "%d/%d", displayIndex, romTotal);
    float textWidth = ui_get_text_width(scrollText);
    ui_draw_text(SCREEN_TOP_WIDTH - textWidth - UI_PADDING, 
                 UI_HEADER_HEIGHT + UI_PADDING, scrollText, UI_COLOR_TEXT_DIM);
    
    // Help text
    ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING,
                 "A: Details | B: Back | L/R: Page", UI_COLOR_TEXT_DIM);
}
