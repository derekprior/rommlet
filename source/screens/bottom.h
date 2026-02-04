/*
 * Bottom screen - Toolbar and debug log modal
 */

#ifndef BOTTOM_H
#define BOTTOM_H

#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>

// Initialize bottom screen module
void bottom_init(void);

// Cleanup bottom screen module
void bottom_exit(void);

// Update bottom screen (handle touch input)
// Returns true if touch was handled
bool bottom_update(void);

// Draw bottom screen
void bottom_draw(void);

// Log a message (replaces printf for app messages)
void bottom_log(const char *fmt, ...);

// Get/set debug level (0=off, 1=requests, 2=bodies)
int bottom_get_debug_level(void);
void bottom_set_debug_level(int level);

#endif // BOTTOM_H
