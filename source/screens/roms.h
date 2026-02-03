/*
 * ROMs screen - Display list of ROMs for a platform
 */

#ifndef ROMS_H
#define ROMS_H

#include <3ds.h>
#include <stdbool.h>
#include "../api.h"

typedef enum {
    ROMS_NONE,
    ROMS_BACK,
    ROMS_SELECTED,
    ROMS_LOAD_MORE
} RomsResult;

// Initialize ROMs screen
void roms_init(void);

// Set ROM data
void roms_set_data(Rom *roms, int count, int total, const char *platformName);

// Append more ROM data (for infinite scroll)
void roms_append_data(Rom *roms, int count);

// Check if more data should be loaded (within threshold of end)
bool roms_needs_more_data(void);

// Get current ROM count (for calculating offset)
int roms_get_count(void);

// Update ROMs screen, returns result
// selectedIndex is set when ROMS_SELECTED is returned
RomsResult roms_update(u32 kDown, int *selectedIndex);

// Draw ROMs screen
void roms_draw(void);

#endif // ROMS_H
