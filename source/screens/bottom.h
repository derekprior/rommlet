/*
 * Bottom screen - Toolbar and debug log modal
 */

#ifndef BOTTOM_H
#define BOTTOM_H

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>
#include "../log.h"

// Bottom screen modes (matches app states that need custom UI)
typedef enum {
    BOTTOM_MODE_DEFAULT,
    BOTTOM_MODE_SETTINGS,
    BOTTOM_MODE_ROM_DETAIL
} BottomMode;

// Bottom screen action results
typedef enum {
    BOTTOM_ACTION_NONE,
    BOTTOM_ACTION_SAVE_SETTINGS,
    BOTTOM_ACTION_CANCEL_SETTINGS,
    BOTTOM_ACTION_OPEN_SETTINGS,
    BOTTOM_ACTION_DOWNLOAD_ROM
} BottomAction;

// Initialize bottom screen module
void bottom_init(void);

// Cleanup bottom screen module
void bottom_exit(void);

// Set the current mode (call when app state changes)
void bottom_set_mode(BottomMode mode);
void bottom_set_settings_mode(bool canCancel);

// Update bottom screen (handle touch input)
// Returns action if a button was pressed
BottomAction bottom_update(void);

// Draw bottom screen
void bottom_draw(void);

// Log subscriber for the debug UI
// Register with log_subscribe(&bottom_log_subscriber)
void bottom_log_subscriber(LogLevel level, const char *message);

#endif // BOTTOM_H
