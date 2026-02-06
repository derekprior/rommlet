/*
 * ROMs screen - Display list of ROMs for a platform
 */

#include "roms.h"
#include "../ui.h"
#include "../listnav.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Rom *romList = NULL;
static char currentPlatform[128] = "";
static ListNav nav;

void roms_init(void) {
    romList = NULL;
    currentPlatform[0] = '\0';
    listnav_reset(&nav);
}

void roms_clear(void) {
    if (romList) {
        free(romList);
        romList = NULL;
    }
    currentPlatform[0] = '\0';
    listnav_reset(&nav);
}

void roms_set_data(Rom *roms, int count, int total, const char *platformName) {
    if (romList) {
        free(romList);
    }
    romList = roms;
    nav.count = count;
    nav.total = total;
    nav.selectedIndex = 0;
    nav.scrollOffset = 0;
    snprintf(currentPlatform, sizeof(currentPlatform), "%s", platformName);
}

void roms_append_data(Rom *roms, int count) {
    if (!roms || count == 0) return;

    Rom *newList = realloc(romList, (nav.count + count) * sizeof(Rom));
    if (!newList) {
        free(roms);
        return;
    }

    romList = newList;
    memcpy(&romList[nav.count], roms, count * sizeof(Rom));
    nav.count += count;
    free(roms);
}

bool roms_needs_more_data(void) {
    return listnav_on_load_more(&nav);
}

int roms_get_count(void) {
    return nav.count;
}

int roms_get_id_at(int index) {
    if (!romList || index < 0 || index >= nav.count) {
        return -1;
    }
    return romList[index].id;
}

const Rom *roms_get_at(int index) {
    if (!romList || index < 0 || index >= nav.count) {
        return NULL;
    }
    return &romList[index];
}

int roms_get_selected_index(void) {
    return nav.selectedIndex;
}

RomsResult roms_update(u32 kDown, int *outSelectedIndex) {
    if (kDown & KEY_B) {
        return ROMS_BACK;
    }

    if (!romList || nav.count == 0) {
        return ROMS_NONE;
    }

    listnav_update(&nav, kDown);

    if (kDown & KEY_A) {
        if (nav.selectedIndex < nav.count) {
            if (outSelectedIndex) *outSelectedIndex = nav.selectedIndex;
            return ROMS_SELECTED;
        }
    }

    if (listnav_on_load_more(&nav)) {
        return ROMS_LOAD_MORE;
    }

    return ROMS_NONE;
}

void roms_draw(void) {
    char headerText[192];
    snprintf(headerText, sizeof(headerText), "ROMs - %s", currentPlatform);
    ui_draw_header(headerText);

    if (!romList || nav.count == 0) {
        ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT / 2, "No ROMs found for this platform.", UI_COLOR_TEXT_DIM);
        ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING, "B: Back to Platforms",
                     UI_COLOR_TEXT_DIM);
        return;
    }

    float y = UI_HEADER_HEIGHT + UI_PADDING;
    float itemWidth = SCREEN_TOP_WIDTH - (UI_PADDING * 2);

    int start, end;
    listnav_visible_range(&nav, &start, &end);

    for (int i = start; i < end; i++) {
        if (i < nav.count) {
            ui_draw_list_item(UI_PADDING, y, itemWidth, romList[i].name, i == nav.selectedIndex);
        } else {
            bool selected = (i == nav.selectedIndex);
            if (selected) {
                ui_draw_rect(UI_PADDING, y, itemWidth, UI_LINE_HEIGHT, UI_COLOR_SELECTED);
            }
            ui_draw_text(UI_PADDING + UI_PADDING, y + 2, "Load more...", selected ? UI_COLOR_TEXT : UI_COLOR_TEXT_DIM);
        }
        y += UI_LINE_HEIGHT;
    }

    listnav_draw_scroll_indicator(&nav);

    ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING, "A: Details | B: Back | L/R: Page",
                 UI_COLOR_TEXT_DIM);
}
