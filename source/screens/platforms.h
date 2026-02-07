/*
 * Platforms screen - Display list of platforms from RomM
 */

#ifndef PLATFORMS_H
#define PLATFORMS_H

#include <3ds.h>
#include "../api.h"

typedef enum { PLATFORMS_NONE, PLATFORMS_SELECTED } PlatformsResult;

// Initialize platforms screen
void platforms_init(void);

// Set platform data
void platforms_set_data(Platform *platforms, int count);

// Update platforms screen, returns result and selected index
PlatformsResult platforms_update(u32 kDown, int *selectedIndex);

// Draw platforms screen
void platforms_draw(void);

#endif // PLATFORMS_H
