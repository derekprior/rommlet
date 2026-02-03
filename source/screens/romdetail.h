/*
 * ROM detail screen - Display ROM information
 */

#ifndef ROMDETAIL_H
#define ROMDETAIL_H

#include <3ds.h>
#include "../api.h"

typedef enum {
    ROMDETAIL_NONE,
    ROMDETAIL_BACK
} RomDetailResult;

// Initialize ROM detail screen
void romdetail_init(void);

// Set ROM detail data
void romdetail_set_data(RomDetail *detail);

// Update ROM detail screen, returns result
RomDetailResult romdetail_update(u32 kDown);

// Draw ROM detail screen
void romdetail_draw(void);

#endif // ROMDETAIL_H
