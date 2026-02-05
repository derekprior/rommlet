/*
 * API module - RomM API wrapper with HTTP and JSON parsing
 */

#ifndef API_H
#define API_H

#include <stdbool.h>

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

// Detailed ROM data from /api/roms/{id}
typedef struct {
    int id;
    int platformId;
    char name[256];
    char fileName[256];
    char summary[1024];
    char platformName[128];
    char firstReleaseDate[32];
    char md5Hash[64];
    char pathCoverSmall[512];
} RomDetail;

// Initialize API module
void api_init(void);

// Cleanup API module
void api_exit(void);

// Set base URL for API requests
void api_set_base_url(const char *url);

// Set authentication credentials (HTTP Basic Auth)
void api_set_auth(const char *username, const char *password);

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

// Fetch single ROM details
// Returns ROM detail, caller must free with api_free_rom_detail
RomDetail *api_get_rom_detail(int romId);

// Free ROM detail
void api_free_rom_detail(RomDetail *detail);

// Download a ROM file to the specified path
// fileName is the fs_name from the ROM detail (used in URL path)
// Returns true on success, false on failure
bool api_download_rom(int romId, const char *fileName, const char *destPath);

#endif // API_H
