/*
 * Queue screen - Display queued ROMs for batch download
 */

#include "queuescreen.h"
#include "../ui.h"
#include "../queue.h"
#include <stdio.h>
#include <string.h>

static int selectedIndex = 0;
static int scrollOffset = 0;

void queue_screen_init(void) {
    selectedIndex = 0;
    scrollOffset = 0;
}

QueueResult queue_screen_update(u32 kDown, int *outSelectedIndex) {
    if (kDown & KEY_B) {
        return QUEUE_BACK;
    }

    int count = queue_count();
    if (count == 0) return QUEUE_NONE;

    // Clamp selection if queue shrank
    if (selectedIndex >= count) {
        selectedIndex = count - 1;
        if (selectedIndex < 0) selectedIndex = 0;
    }

    // Navigation
    if (kDown & KEY_DOWN) {
        selectedIndex++;
        if (selectedIndex >= count) {
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
            selectedIndex = count - 1;
            scrollOffset = count > UI_VISIBLE_ITEMS ? count - UI_VISIBLE_ITEMS : 0;
        }
        if (selectedIndex < scrollOffset) {
            scrollOffset = selectedIndex;
        }
    }

    // Page up/down
    if (kDown & KEY_R) {
        selectedIndex += UI_VISIBLE_ITEMS;
        if (selectedIndex >= count) selectedIndex = count - 1;
        if (selectedIndex >= scrollOffset + UI_VISIBLE_ITEMS) {
            scrollOffset = selectedIndex - UI_VISIBLE_ITEMS + 1;
        }
    }

    if (kDown & KEY_L) {
        selectedIndex -= UI_VISIBLE_ITEMS;
        if (selectedIndex < 0) selectedIndex = 0;
        if (selectedIndex < scrollOffset) {
            scrollOffset = selectedIndex;
        }
    }

    // Select item
    if (kDown & KEY_A) {
        if (outSelectedIndex) *outSelectedIndex = selectedIndex;
        return QUEUE_SELECTED;
    }

    return QUEUE_NONE;
}

void queue_screen_draw(void) {
    ui_draw_header("Download Queue");

    int count = queue_count();

    if (count == 0) {
        ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT / 2, 
                     "No ROMs queued", UI_COLOR_TEXT_DIM);
        ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING,
                     "B: Back", UI_COLOR_TEXT_DIM);
        return;
    }

    float y = UI_HEADER_HEIGHT + UI_PADDING;
    float itemWidth = SCREEN_TOP_WIDTH - (UI_PADDING * 2);

    int visibleEnd = scrollOffset + UI_VISIBLE_ITEMS;
    if (visibleEnd > count) visibleEnd = count;

    for (int i = scrollOffset; i < visibleEnd; i++) {
        QueueEntry *entry = queue_get(i);
        if (!entry) continue;

        // Build display text: "[slug] name"
        char displayText[384];
        snprintf(displayText, sizeof(displayText), "[%s] %s", 
                 entry->platformSlug, entry->name);

        bool selected = (i == selectedIndex);

        if (entry->failed) {
            // Failed items: red X prefix
            if (selected) {
                ui_draw_rect(UI_PADDING, y, itemWidth, UI_LINE_HEIGHT, UI_COLOR_SELECTED);
            }
            char failText[400];
            snprintf(failText, sizeof(failText), "X %s", displayText);
            ui_draw_text(UI_PADDING + UI_PADDING, y + 2, failText,
                         C2D_Color32(0xFF, 0x44, 0x44, 0xFF));
        } else {
            ui_draw_list_item(UI_PADDING, y, itemWidth, displayText, selected);
        }
        y += UI_LINE_HEIGHT;
    }

    // Scroll indicator
    char scrollText[32];
    snprintf(scrollText, sizeof(scrollText), "%d/%d", selectedIndex + 1, count);
    float textWidth = ui_get_text_width(scrollText);
    ui_draw_text(SCREEN_TOP_WIDTH - textWidth - UI_PADDING,
                 UI_HEADER_HEIGHT + UI_PADDING, scrollText, UI_COLOR_TEXT_DIM);

    // Help text
    ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING,
                 "A: Details | B: Back | L/R: Page", UI_COLOR_TEXT_DIM);
}
