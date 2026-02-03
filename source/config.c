/*
 * Config module - Load/save settings to SD card
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <3ds.h>

void config_init(Config *config) {
    memset(config, 0, sizeof(Config));
    strncpy(config->serverUrl, "http://pinbox.local", CONFIG_MAX_URL_LEN - 1);
    strncpy(config->romFolder, "sdmc:/roms", CONFIG_MAX_PATH_LEN - 1);
}

bool config_load(Config *config) {
    FILE *f = fopen(CONFIG_PATH, "r");
    if (!f) {
        return false;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        // Remove newline
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        newline = strchr(line, '\r');
        if (newline) *newline = '\0';
        
        // Parse key=value
        char *eq = strchr(line, '=');
        if (!eq) continue;
        
        *eq = '\0';
        char *key = line;
        char *value = eq + 1;
        
        if (strcmp(key, "serverUrl") == 0) {
            strncpy(config->serverUrl, value, CONFIG_MAX_URL_LEN - 1);
        } else if (strcmp(key, "username") == 0) {
            strncpy(config->username, value, CONFIG_MAX_USER_LEN - 1);
        } else if (strcmp(key, "password") == 0) {
            strncpy(config->password, value, CONFIG_MAX_PASS_LEN - 1);
        } else if (strcmp(key, "romFolder") == 0) {
            strncpy(config->romFolder, value, CONFIG_MAX_PATH_LEN - 1);
        }
    }
    
    fclose(f);
    return config_is_valid(config);
}

bool config_save(const Config *config) {
    // Ensure directory exists
    mkdir(CONFIG_DIR, 0755);
    
    FILE *f = fopen(CONFIG_PATH, "w");
    if (!f) {
        return false;
    }
    
    fprintf(f, "serverUrl=%s\n", config->serverUrl);
    fprintf(f, "username=%s\n", config->username);
    fprintf(f, "password=%s\n", config->password);
    fprintf(f, "romFolder=%s\n", config->romFolder);
    
    fclose(f);
    return true;
}

bool config_is_valid(const Config *config) {
    return config->serverUrl[0] != '\0' &&
           config->username[0] != '\0' &&
           config->password[0] != '\0' &&
           config->romFolder[0] != '\0';
}
