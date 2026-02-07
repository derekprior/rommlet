/*
 * Platforms screen - Display list of platforms from RomM
 */

#include "platforms.h"
#include "../ui.h"
#include "../listnav.h"
#include <stdio.h>
#include <string.h>

static Platform *platformList = NULL;
static ListNav nav;

void platforms_init(void) {
    platformList = NULL;
    listnav_reset(&nav);
}

void platforms_set_data(Platform *platforms, int count) {
    platformList = platforms;
    listnav_set(&nav, count, count);
}

PlatformsResult platforms_update(u32 kDown, int *outSelectedIndex) {
    if (!platformList || nav.count == 0) {
        return PLATFORMS_NONE;
    }

    listnav_update(&nav, kDown);

    if (kDown & KEY_A) {
        if (outSelectedIndex) *outSelectedIndex = nav.selectedIndex;
        return PLATFORMS_SELECTED;
    }

    return PLATFORMS_NONE;
}

void platforms_draw(void) {
    ui_draw_header("Platforms");

    if (!platformList || nav.count == 0) {
        ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT / 2, "No platforms found.", UI_COLOR_TEXT_DIM);
        return;
    }

    float y = UI_HEADER_HEIGHT + UI_PADDING;
    float itemWidth = SCREEN_TOP_WIDTH - (UI_PADDING * 2);

    int start, end;
    listnav_visible_range(&nav, &start, &end);

    for (int i = start; i < end; i++) {
        char itemText[192];
        snprintf(itemText, sizeof(itemText), "%s (%d ROMs)", platformList[i].displayName, platformList[i].romCount);

        ui_draw_list_item(UI_PADDING, y, itemWidth, itemText, i == nav.selectedIndex);
        y += UI_LINE_HEIGHT;
    }

    if (nav.count > UI_VISIBLE_ITEMS) {
        listnav_draw_scroll_indicator(&nav);
    }

    ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING, "A: Select", UI_COLOR_TEXT_DIM);
}
