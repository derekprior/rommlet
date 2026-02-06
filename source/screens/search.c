/*
 * Search screen - Search for ROMs across platforms
 */

#include "search.h"
#include "../ui.h"
#include "../listnav.h"
#include "../log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Search form state
static char searchTerm[256] = "";
static Platform *platformList = NULL;
static int platformCount = 0;
static bool platformSelected[SEARCH_MAX_PLATFORMS];
static int platformScrollOffset = 0;
static int platformCursorIndex = 0;

// Search results state
static Rom *resultList = NULL;
static ListNav nav;

// Platform list layout
#define TOOLBAR_HEIGHT 24
#define FORM_FIELD_Y (TOOLBAR_HEIGHT + UI_PADDING)
#define FORM_FIELD_HEIGHT 22
#define PLATFORM_LIST_Y (FORM_FIELD_Y + FORM_FIELD_HEIGHT + UI_PADDING + UI_LINE_HEIGHT + UI_PADDING)
#define PLATFORM_ITEM_HEIGHT 18

// How many platform items fit on screen (above search button)
#define SEARCH_BUTTON_HEIGHT 30
#define PLATFORM_LIST_BOTTOM (SCREEN_BOTTOM_HEIGHT - SEARCH_BUTTON_HEIGHT - UI_PADDING * 2 - UI_LINE_HEIGHT)
#define PLATFORM_VISIBLE ((int)((PLATFORM_LIST_BOTTOM - PLATFORM_LIST_Y) / PLATFORM_ITEM_HEIGHT))

void search_init(Platform *platforms, int count) {
    platformList = platforms;
    platformCount = count > SEARCH_MAX_PLATFORMS ? SEARCH_MAX_PLATFORMS : count;
    platformScrollOffset = 0;
    platformCursorIndex = 0;
    searchTerm[0] = '\0';
    
    // Default: all selected
    for (int i = 0; i < platformCount; i++) {
        platformSelected[i] = true;
    }
    
    // Clear results
    if (resultList) {
        free(resultList);
        resultList = NULL;
    }
    listnav_reset(&nav);
}

const char *search_get_term(void) {
    return searchTerm;
}

const int *search_get_platform_ids(int *count) {
    static int ids[SEARCH_MAX_PLATFORMS];
    int n = 0;
    bool allSelected = true;
    for (int i = 0; i < platformCount; i++) {
        if (platformSelected[i]) {
            ids[n++] = platformList[i].id;
        } else {
            allSelected = false;
        }
    }
    // If all selected, return 0 count to omit platform filter
    if (allSelected) {
        *count = 0;
        return NULL;
    }
    *count = n;
    return ids;
}

void search_set_results(Rom *roms, int count, int total) {
    if (resultList) {
        free(resultList);
    }
    resultList = roms;
    nav.count = count;
    nav.total = total;
    nav.selectedIndex = 0;
    nav.scrollOffset = 0;
}

void search_append_results(Rom *roms, int count) {
    if (!roms || count == 0) return;
    
    Rom *newList = realloc(resultList, (nav.count + count) * sizeof(Rom));
    if (!newList) {
        free(roms);
        return;
    }
    
    resultList = newList;
    memcpy(&resultList[nav.count], roms, count * sizeof(Rom));
    nav.count += count;
    free(roms);
}

int search_get_result_count(void) {
    return nav.count;
}

const Rom *search_get_result_at(int index) {
    if (!resultList || index < 0 || index >= nav.count) {
        return NULL;
    }
    return &resultList[index];
}

int search_get_selected_index(void) {
    return nav.selectedIndex;
}

int search_get_result_id_at(int index) {
    if (!resultList || index < 0 || index >= nav.count) {
        return -1;
    }
    return resultList[index].id;
}

const char *search_get_platform_name(int platformId) {
    for (int i = 0; i < platformCount; i++) {
        if (platformList[i].id == platformId) {
            return platformList[i].displayName;
        }
    }
    return "Unknown";
}

const char *search_get_platform_slug(int platformId) {
    for (int i = 0; i < platformCount; i++) {
        if (platformList[i].id == platformId) {
            return platformList[i].slug;
        }
    }
    return "";
}

void search_open_keyboard(void) {
    ui_show_keyboard("Search ROMs...", searchTerm, sizeof(searchTerm), false);
}

SearchFormResult search_form_update(u32 kDown) {
    if (kDown & KEY_B) {
        return SEARCH_FORM_BACK;
    }
    
    // D-pad navigates platform list
    if (kDown & KEY_DOWN) {
        platformCursorIndex++;
        if (platformCursorIndex >= platformCount) {
            platformCursorIndex = 0;
            platformScrollOffset = 0;
        }
        if (platformCursorIndex >= platformScrollOffset + PLATFORM_VISIBLE) {
            platformScrollOffset = platformCursorIndex - PLATFORM_VISIBLE + 1;
        }
    }
    
    if (kDown & KEY_UP) {
        platformCursorIndex--;
        if (platformCursorIndex < 0) {
            platformCursorIndex = platformCount - 1;
            platformScrollOffset = platformCount > PLATFORM_VISIBLE ? platformCount - PLATFORM_VISIBLE : 0;
        }
        if (platformCursorIndex < platformScrollOffset) {
            platformScrollOffset = platformCursorIndex;
        }
    }
    
    // A toggles platform selection
    if (kDown & KEY_A) {
        if (platformCursorIndex >= 0 && platformCursorIndex < platformCount) {
            platformSelected[platformCursorIndex] = !platformSelected[platformCursorIndex];
        }
    }
    
    // L = select none, R = select all
    if (kDown & KEY_L) {
        for (int i = 0; i < platformCount; i++) {
            platformSelected[i] = false;
        }
    }
    if (kDown & KEY_R) {
        for (int i = 0; i < platformCount; i++) {
            platformSelected[i] = true;
        }
    }
    
    // X triggers search
    if ((kDown & KEY_X) && searchTerm[0]) {
        return SEARCH_FORM_EXECUTE;
    }
    
    return SEARCH_FORM_NONE;
}

void search_form_draw(void) {
    // Search field (tappable)
    ui_draw_rect(UI_PADDING, FORM_FIELD_Y, SCREEN_BOTTOM_WIDTH - UI_PADDING * 2, FORM_FIELD_HEIGHT,
                 C2D_Color32(0x30, 0x30, 0x48, 0xFF));
    if (searchTerm[0]) {
        ui_draw_text(UI_PADDING + 4, FORM_FIELD_Y + 3, searchTerm, UI_COLOR_TEXT);
    } else {
        ui_draw_text(UI_PADDING + 4, FORM_FIELD_Y + 3, "Tap to enter search term...", UI_COLOR_TEXT_DIM);
    }
    
    // Platform filter header
    float headerY = FORM_FIELD_Y + FORM_FIELD_HEIGHT + UI_PADDING;
    ui_draw_text(UI_PADDING, headerY, "Platforms:", UI_COLOR_TEXT_DIM);
    
    // Select all/none hint
    const char *hint = "L: None  R: All";
    float hintWidth = ui_get_text_width(hint);
    ui_draw_text(SCREEN_BOTTOM_WIDTH - hintWidth - UI_PADDING, headerY, hint, UI_COLOR_TEXT_DIM);
    
    // Platform checklist
    float y = PLATFORM_LIST_Y;
    int visibleEnd = platformScrollOffset + PLATFORM_VISIBLE;
    if (visibleEnd > platformCount) visibleEnd = platformCount;
    
    for (int i = platformScrollOffset; i < visibleEnd; i++) {
        bool isCursor = (i == platformCursorIndex);
        
        // Highlight
        if (isCursor) {
            ui_draw_rect(UI_PADDING, y, SCREEN_BOTTOM_WIDTH - UI_PADDING * 2, PLATFORM_ITEM_HEIGHT, UI_COLOR_SELECTED);
        }
        
        // Checkbox
        const char *check = platformSelected[i] ? "[x] " : "[ ] ";
        char line[196];
        snprintf(line, sizeof(line), "%s%s", check, platformList[i].displayName);
        ui_draw_text(UI_PADDING + 4, y + 1, line, isCursor ? UI_COLOR_TEXT : UI_COLOR_TEXT_DIM);
        
        y += PLATFORM_ITEM_HEIGHT;
    }
    
    // Scroll indicator for platform list
    if (platformCount > PLATFORM_VISIBLE) {
        char scrollText[32];
        snprintf(scrollText, sizeof(scrollText), "%d/%d", platformCursorIndex + 1, platformCount);
        float tw = ui_get_text_width(scrollText);
        ui_draw_text(SCREEN_BOTTOM_WIDTH - tw - UI_PADDING, PLATFORM_LIST_BOTTOM, scrollText, UI_COLOR_TEXT_DIM);
    }
    
    // Search button
    if (searchTerm[0]) {
        float btnY = SCREEN_BOTTOM_HEIGHT - SEARCH_BUTTON_HEIGHT - UI_PADDING;
        float btnW = 200;
        float btnX = (SCREEN_BOTTOM_WIDTH - btnW) / 2;
        // Simple button
        ui_draw_rect(btnX, btnY, btnW, SEARCH_BUTTON_HEIGHT, UI_COLOR_ACCENT);
        const char *btnLabel = "Search";
        float btnLabelW = ui_get_text_width(btnLabel);
        ui_draw_text(btnX + (btnW - btnLabelW) / 2, btnY + 7, btnLabel, UI_COLOR_TEXT);
    }
    
    // Help text
    float bottomY = SCREEN_BOTTOM_HEIGHT - UI_LINE_HEIGHT - UI_PADDING;
    if (!searchTerm[0]) {
        ui_draw_text(UI_PADDING, bottomY, "A: Toggle | B: Back", UI_COLOR_TEXT_DIM);
    }
}

// Search results - top screen

SearchResultsResult search_results_update(u32 kDown, int *outSelectedIndex) {
    if (kDown & KEY_B) {
        return SEARCH_RESULTS_BACK;
    }
    
    if (!resultList || nav.count == 0) {
        return SEARCH_RESULTS_NONE;
    }
    
    listnav_update(&nav, kDown);
    
    if (kDown & KEY_A) {
        if (nav.selectedIndex < nav.count) {
            if (outSelectedIndex) *outSelectedIndex = nav.selectedIndex;
            return SEARCH_RESULTS_SELECTED;
        }
    }
    
    if (listnav_on_load_more(&nav)) {
        return SEARCH_RESULTS_LOAD_MORE;
    }
    
    return SEARCH_RESULTS_NONE;
}

void search_results_draw(void) {
    char headerText[320];
    snprintf(headerText, sizeof(headerText), "Search: \"%s\"", searchTerm);
    ui_draw_header(headerText);
    
    if (!resultList || nav.count == 0) {
        ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT / 2,
                     "No results found.", UI_COLOR_TEXT_DIM);
        ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING,
                     "B: Back to Search", UI_COLOR_TEXT_DIM);
        return;
    }
    
    float y = UI_HEADER_HEIGHT + UI_PADDING;
    float itemWidth = SCREEN_TOP_WIDTH - (UI_PADDING * 2);
    
    int start, end;
    listnav_visible_range(&nav, &start, &end);
    
    for (int i = start; i < end; i++) {
        if (i < nav.count) {
            char displayText[512];
            const char *slug = search_get_platform_slug(resultList[i].platformId);
            snprintf(displayText, sizeof(displayText), "[%s] %s", slug, resultList[i].name);
            ui_draw_list_item(UI_PADDING, y, itemWidth, displayText, i == nav.selectedIndex);
        } else {
            bool selected = (i == nav.selectedIndex);
            if (selected) {
                ui_draw_rect(UI_PADDING, y, itemWidth, UI_LINE_HEIGHT, UI_COLOR_SELECTED);
            }
            ui_draw_text(UI_PADDING + UI_PADDING, y + 2, "Load more...",
                         selected ? UI_COLOR_TEXT : UI_COLOR_TEXT_DIM);
        }
        y += UI_LINE_HEIGHT;
    }
    
    listnav_draw_scroll_indicator(&nav);
    
    ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING,
                 "A: Details | B: Back | L/R: Page", UI_COLOR_TEXT_DIM);
}
