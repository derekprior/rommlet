/*
 * API module - RomM API wrapper with HTTP and JSON parsing
 */

#ifndef API_H
#define API_H

#include <stdbool.h>

// Debug levels
#define API_DEBUG_OFF      0  // No debug output
#define API_DEBUG_REQUESTS 1  // Log URLs, status codes, sizes
#define API_DEBUG_BODIES   2  // Log request info + response bodies

// Platform data from /api/platforms
typedef struct {
    int id;
    char slug[64];
    char name[128];
    char displayName[128];
    int romCount;
} Platform;

// ROM data from /api/roms
typedef struct {
    int id;
    int platformId;
    char name[256];
    char fsName[256];
    char pathCoverSmall[512];
} Rom;

// Initialize API module
void api_init(void);

// Cleanup API module
void api_exit(void);

// Set base URL for API requests
void api_set_base_url(const char *url);

// Set authentication credentials (HTTP Basic Auth)
void api_set_auth(const char *username, const char *password);

// Debug level control
void api_set_debug_level(int level);
int api_get_debug_level(void);

// Fetch platforms from server
// Returns array of platforms, sets count. Caller must free with api_free_platforms
Platform *api_get_platforms(int *count);

// Free platforms array
void api_free_platforms(Platform *platforms, int count);

// Fetch ROMs for a platform
// Returns array of ROMs, sets count and total. Caller must free with api_free_roms
Rom *api_get_roms(int platformId, int offset, int limit, int *count, int *total);

// Free ROMs array
void api_free_roms(Rom *roms, int count);

#endif // API_H
