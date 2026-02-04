/*
 * Config module - Load/save settings to SD card
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#define CONFIG_MAX_URL_LEN 256
#define CONFIG_MAX_USER_LEN 64
#define CONFIG_MAX_PASS_LEN 64
#define CONFIG_MAX_PATH_LEN 256
#define CONFIG_MAX_SLUG_LEN 64
#define CONFIG_PATH "sdmc:/3ds/rommlet/config.ini"
#define CONFIG_MAPPINGS_PATH "sdmc:/3ds/rommlet/mappings.ini"
#define CONFIG_DIR "sdmc:/3ds/rommlet"

typedef struct {
    char serverUrl[CONFIG_MAX_URL_LEN];
    char username[CONFIG_MAX_USER_LEN];
    char password[CONFIG_MAX_PASS_LEN];
    char romFolder[CONFIG_MAX_PATH_LEN];
} Config;

// Initialize config with defaults
void config_init(Config *config);

// Load config from SD card, returns true if successful
bool config_load(Config *config);

// Save config to SD card, returns true if successful
bool config_save(const Config *config);

// Check if config has required fields
bool config_is_valid(const Config *config);

// Platform folder mappings (slug -> subfolder name)
// Get subfolder for a platform slug, returns NULL if not mapped
const char *config_get_platform_folder(const char *platformSlug);

// Set subfolder for a platform slug
bool config_set_platform_folder(const char *platformSlug, const char *folderName);

#endif // CONFIG_H
