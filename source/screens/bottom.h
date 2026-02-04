/*
 * Bottom screen - Toolbar and debug log modal
 */

#ifndef BOTTOM_H
#define BOTTOM_H

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>

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

// Log a message (replaces printf for app messages)
void bottom_log(const char *fmt, ...);

// Get/set debug level (0=off, 1=requests, 2=bodies)
int bottom_get_debug_level(void);
void bottom_set_debug_level(int level);

#endif // BOTTOM_H
