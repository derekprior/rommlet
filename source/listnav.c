/*
 * ListNav - Shared list navigation and rendering for scrollable lists
 */

#include "listnav.h"
#include "ui.h"
#include <stdio.h>

void listnav_reset(ListNav *nav) {
    nav->selectedIndex = 0;
    nav->scrollOffset = 0;
    nav->count = 0;
    nav->total = 0;
    nav->visibleItems = 0;
}

// Display count includes "Load more" row when count < total
static int display_count(const ListNav *nav) {
    return nav->count + (nav->count < nav->total ? 1 : 0);
}

static int visible_items(const ListNav *nav) {
    return nav->visibleItems > 0 ? nav->visibleItems : UI_VISIBLE_ITEMS;
}

bool listnav_update(ListNav *nav, u32 kDown) {
    int dc = display_count(nav);
    if (dc == 0) return false;

    int prev = nav->selectedIndex;
    int vis = visible_items(nav);

    if (kDown & KEY_DOWN) {
        nav->selectedIndex++;
        if (nav->selectedIndex >= dc) {
            nav->selectedIndex = 0;
            nav->scrollOffset = 0;
        }
        if (nav->selectedIndex >= nav->scrollOffset + vis) {
            nav->scrollOffset = nav->selectedIndex - vis + 1;
        }
    }

    if (kDown & KEY_UP) {
        nav->selectedIndex--;
        if (nav->selectedIndex < 0) {
            nav->selectedIndex = dc - 1;
            nav->scrollOffset = dc > vis ? dc - vis : 0;
        }
        if (nav->selectedIndex < nav->scrollOffset) {
            nav->scrollOffset = nav->selectedIndex;
        }
    }

    if (kDown & KEY_R) {
        nav->selectedIndex += vis;
        if (nav->selectedIndex >= dc) {
            nav->selectedIndex = dc - 1;
        }
        if (nav->selectedIndex >= nav->scrollOffset + vis) {
            nav->scrollOffset = nav->selectedIndex - vis + 1;
        }
    }

    if (kDown & KEY_L) {
        nav->selectedIndex -= vis;
        if (nav->selectedIndex < 0) {
            nav->selectedIndex = 0;
        }
        if (nav->selectedIndex < nav->scrollOffset) {
            nav->scrollOffset = nav->selectedIndex;
        }
    }

    return nav->selectedIndex != prev;
}

void listnav_visible_range(const ListNav *nav, int *start, int *end) {
    int dc = display_count(nav);
    int vis = visible_items(nav);
    *start = nav->scrollOffset;
    *end = nav->scrollOffset + vis;
    if (*end > dc) *end = dc;
}

void listnav_draw_scroll_indicator(const ListNav *nav) {
    char scrollText[64];
    int displayIndex = nav->selectedIndex < nav->count ? nav->selectedIndex + 1 : nav->count;
    snprintf(scrollText, sizeof(scrollText), "%d/%d", displayIndex, nav->total);
    float textWidth = ui_get_text_width(scrollText);
    ui_draw_text(SCREEN_TOP_WIDTH - textWidth - UI_PADDING, UI_HEADER_HEIGHT + UI_PADDING, scrollText,
                 UI_COLOR_TEXT_DIM);
}

bool listnav_on_load_more(const ListNav *nav) {
    return nav->count < nav->total && nav->selectedIndex == nav->count;
}
