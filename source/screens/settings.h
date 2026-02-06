/*
 * Settings screen - Configure server connection
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <3ds.h>
#include "../config.h"

typedef enum { SETTINGS_NONE, SETTINGS_SAVED, SETTINGS_CANCELLED } SettingsResult;

// Initialize settings screen
void settings_init(Config *config);

// Update settings screen, returns result
SettingsResult settings_update(u32 kDown);

// Draw settings screen
void settings_draw(void);

#endif // SETTINGS_H
