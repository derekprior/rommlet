/*
 * ListNav - Shared list navigation and rendering for scrollable lists
 */

#ifndef LISTNAV_H
#define LISTNAV_H

#include <3ds.h>
#include <stdbool.h>

// Navigable list state
typedef struct {
    int selectedIndex;
    int scrollOffset;
    int count;        // actual items
    int total;        // total available (count < total means "load more" row exists)
    int visibleItems; // items visible on screen (0 = use UI_VISIBLE_ITEMS default)
} ListNav;

// Reset navigation state to defaults
void listnav_reset(ListNav *nav);

// Handle d-pad and L/R input. Returns true if selection changed.
bool listnav_update(ListNav *nav, u32 kDown);

// Get visible range [start, end) for rendering
void listnav_visible_range(const ListNav *nav, int *start, int *end);

// Draw scroll indicator (e.g. "3/50") in top-right corner
void listnav_draw_scroll_indicator(const ListNav *nav);

// Whether the cursor is on the virtual "Load more" row
bool listnav_on_load_more(const ListNav *nav);

#endif // LISTNAV_H
