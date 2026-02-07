/*
 * Config module - Load/save settings to SD card
 */

#include "config.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <3ds.h>

// Platform folder mappings cache
#define MAX_MAPPINGS 64
typedef struct {
    char slug[CONFIG_MAX_SLUG_LEN];
    char folder[CONFIG_MAX_SLUG_LEN];
} PlatformMapping;

static PlatformMapping mappings[MAX_MAPPINGS];
static int mappingCount = 0;
static bool mappingsLoaded = false;

void config_init(Config *config) {
    memset(config, 0, sizeof(Config));
    config->serverUrl[0] = '\0';
    snprintf(config->romFolder, CONFIG_MAX_PATH_LEN, "sdmc:/roms");
}

bool config_load(Config *config) {
    FILE *f = fopen(CONFIG_PATH, "r");
    if (!f) {
        return false;
    }

    // Reset mappings when loading config
    mappingCount = 0;
    mappingsLoaded = true;

    char line[512];
    bool inMappingsSection = false;

    while (fgets(line, sizeof(line), f)) {
        // Remove newline
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        newline = strchr(line, '\r');
        if (newline) *newline = '\0';

        // Skip empty lines
        if (line[0] == '\0') continue;

        // Check for section headers
        if (line[0] == '[') {
            inMappingsSection = (strcmp(line, "[platform_mappings]") == 0);
            continue;
        }

        // Parse key=value
        char *eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = line;
        char *value = eq + 1;

        if (inMappingsSection) {
            // Platform mapping entry
            if (mappingCount < MAX_MAPPINGS) {
                snprintf(mappings[mappingCount].slug, CONFIG_MAX_SLUG_LEN, "%.63s", key);
                snprintf(mappings[mappingCount].folder, CONFIG_MAX_SLUG_LEN, "%.63s", value);
                mappingCount++;
            }
        } else {
            // Main config entries
            if (strcmp(key, "serverUrl") == 0) {
                snprintf(config->serverUrl, CONFIG_MAX_URL_LEN, "%s", value);
            } else if (strcmp(key, "username") == 0) {
                snprintf(config->username, CONFIG_MAX_USER_LEN, "%s", value);
            } else if (strcmp(key, "password") == 0) {
                snprintf(config->password, CONFIG_MAX_PASS_LEN, "%s", value);
            } else if (strcmp(key, "romFolder") == 0) {
                snprintf(config->romFolder, CONFIG_MAX_PATH_LEN, "%s", value);
            }
        }
    }

    fclose(f);
    return config_is_valid(config);
}

static bool save_config_file(const Config *config) {
    // Ensure directory exists
    mkdir(CONFIG_DIR, 0755);

    FILE *f = fopen(CONFIG_PATH, "w");
    if (!f) {
        log_error("Failed to open config file for writing: %s", CONFIG_PATH);
        return false;
    }

    // Write main config
    fprintf(f, "serverUrl=%s\n", config->serverUrl);
    fprintf(f, "username=%s\n", config->username);
    fprintf(f, "password=%s\n", config->password);
    fprintf(f, "romFolder=%s\n", config->romFolder);

    // Write platform mappings section
    if (mappingCount > 0) {
        fprintf(f, "\n[platform_mappings]\n");
        for (int i = 0; i < mappingCount; i++) {
            fprintf(f, "%s=%s\n", mappings[i].slug, mappings[i].folder);
        }
    }

    bool ok = !ferror(f);
    if (!ok) {
        log_error("Failed to write config file: %s", CONFIG_PATH);
    }
    fclose(f);
    return ok;
}

bool config_save(const Config *config) {
    return save_config_file(config);
}

bool config_is_valid(const Config *config) {
    return config->serverUrl[0] != '\0' && config->username[0] != '\0' && config->password[0] != '\0' &&
           config->romFolder[0] != '\0';
}

const char *config_get_platform_folder(const char *platformSlug) {
    for (int i = 0; i < mappingCount; i++) {
        if (strcmp(mappings[i].slug, platformSlug) == 0) {
            return mappings[i].folder;
        }
    }
    return NULL;
}

bool config_set_platform_folder(const Config *config, const char *platformSlug, const char *folderName) {
    // Check if mapping already exists
    for (int i = 0; i < mappingCount; i++) {
        if (strcmp(mappings[i].slug, platformSlug) == 0) {
            snprintf(mappings[i].folder, CONFIG_MAX_SLUG_LEN, "%.63s", folderName);
            log_info("Platform '%s' folder set to '%s'", platformSlug, folderName);
            return save_config_file(config);
        }
    }

    // Add new mapping
    if (mappingCount >= MAX_MAPPINGS) return false;

    snprintf(mappings[mappingCount].slug, CONFIG_MAX_SLUG_LEN, "%.63s", platformSlug);
    snprintf(mappings[mappingCount].folder, CONFIG_MAX_SLUG_LEN, "%.63s", folderName);
    mappingCount++;

    log_info("Platform '%s' folder set to '%s'", platformSlug, folderName);
    return save_config_file(config);
}
