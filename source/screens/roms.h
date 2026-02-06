/*
 * ROMs screen - Display list of ROMs for a platform
 */

#ifndef ROMS_H
#define ROMS_H

#include <3ds.h>
#include <stdbool.h>
#include "../api.h"

typedef enum { ROMS_NONE, ROMS_BACK, ROMS_SELECTED, ROMS_LOAD_MORE } RomsResult;

// Initialize ROMs screen
void roms_init(void);

// Clear ROM data and free memory
void roms_clear(void);

// Set ROM data (takes ownership of roms pointer)
void roms_set_data(Rom *roms, int count, int total, const char *platformName);

// Append more ROM data (for infinite scroll)
void roms_append_data(Rom *roms, int count);

// Check if more data should be loaded (within threshold of end)
bool roms_needs_more_data(void);

// Get current ROM count (for calculating offset)
int roms_get_count(void);

// Get ROM ID at index (returns -1 if invalid)
int roms_get_id_at(int index);

// Get ROM at index (returns NULL if invalid)
const Rom *roms_get_at(int index);

// Get current selected index
int roms_get_selected_index(void);

// Update ROMs screen, returns result
RomsResult roms_update(u32 kDown);

// Draw ROMs screen
void roms_draw(void);

#endif // ROMS_H
