/*
 * Background loader module - Async ROM fetching
 */

#ifndef LOADER_H
#define LOADER_H

#include <stdbool.h>
#include "api.h"

// Initialize the loader module
void loader_init(void);

// Cleanup the loader module
void loader_exit(void);

// Start loading ROMs in background
// Returns false if a load is already in progress
bool loader_start_roms(int platformId, int offset, int limit);

// Check if background load is complete
bool loader_is_complete(void);

// Check if loader is currently working
bool loader_is_busy(void);

// Get loaded ROMs (transfers ownership to caller)
// Returns NULL if not complete or failed
// Sets count and total output parameters
Rom *loader_get_roms(int *count, int *total);

#endif // LOADER_H
