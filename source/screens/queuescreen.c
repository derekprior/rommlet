/*
 * Queue screen - Display queued ROMs for batch download
 */

#include "queuescreen.h"
#include "../ui.h"
#include "../listnav.h"
#include "../queue.h"
#include <stdio.h>
#include <string.h>

static ListNav nav;

void queue_screen_init(void) {
    nav.selectedIndex = 0;
    nav.scrollOffset = 0;
}

QueueResult queue_screen_update(u32 kDown) {
    if (kDown & KEY_B) {
        return QUEUE_BACK;
    }

    nav.count = queue_count();
    nav.total = nav.count;
    if (nav.count == 0) return QUEUE_NONE;

    // Clamp selection if queue shrank
    if (nav.selectedIndex >= nav.count) {
        nav.selectedIndex = nav.count - 1;
        if (nav.selectedIndex < 0) nav.selectedIndex = 0;
    }

    listnav_update(&nav, kDown);

    if (kDown & KEY_A) {
        return QUEUE_SELECTED;
    }

    return QUEUE_NONE;
}

int queue_screen_get_selected_index(void) {
    return nav.selectedIndex;
}

void queue_screen_draw(void) {
    ui_draw_header("Download Queue");

    if (nav.count == 0) {
        const char *emptyMsg = "No ROMs queued";
        float emptyWidth = ui_get_text_width(emptyMsg);
        ui_draw_text((SCREEN_TOP_WIDTH - emptyWidth) / 2, SCREEN_TOP_HEIGHT / 2, emptyMsg, UI_COLOR_TEXT_DIM);
        ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING, "B: Back", UI_COLOR_TEXT_DIM);
        return;
    }

    float y = UI_HEADER_HEIGHT + UI_PADDING;
    float itemWidth = SCREEN_TOP_WIDTH - (UI_PADDING * 2);

    int start, end;
    listnav_visible_range(&nav, &start, &end);

    for (int i = start; i < end; i++) {
        QueueEntry *entry = queue_get(i);
        if (!entry) continue;

        char displayText[384];
        snprintf(displayText, sizeof(displayText), "[%s] %s", entry->platformSlug, entry->name);

        bool selected = (i == nav.selectedIndex);

        if (entry->failed) {
            if (selected) {
                ui_draw_rect(UI_PADDING, y, itemWidth, UI_LINE_HEIGHT, UI_COLOR_SELECTED);
            }
            char failText[400];
            snprintf(failText, sizeof(failText), "X %s", displayText);
            ui_draw_text(UI_PADDING + UI_PADDING, y + 2, failText, C2D_Color32(0xFF, 0x44, 0x44, 0xFF));
        } else {
            ui_draw_list_item(UI_PADDING, y, itemWidth, displayText, selected);
        }
        y += UI_LINE_HEIGHT;
    }

    listnav_draw_scroll_indicator(&nav);

    ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING, "A: Details | B: Back | L/R: Page",
                 UI_COLOR_TEXT_DIM);
}
