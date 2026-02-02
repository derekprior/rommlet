/*
 * ROMs screen - Display list of ROMs for a platform
 */

#ifndef ROMS_H
#define ROMS_H

#include <3ds.h>
#include "../api.h"

typedef enum {
    ROMS_NONE,
    ROMS_BACK,
    ROMS_SELECTED
} RomsResult;

// Initialize ROMs screen
void roms_init(void);

// Set ROM data
void roms_set_data(Rom *roms, int count, int total, const char *platformName);

// Update ROMs screen, returns result
RomsResult roms_update(u32 kDown);

// Draw ROMs screen
void roms_draw(void);

#endif // ROMS_H
