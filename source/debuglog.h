/*
 * Debug log viewer - Scrollable modal overlay for log messages
 */

#ifndef DEBUGLOG_H
#define DEBUGLOG_H

#include <stdbool.h>
#include "log.h"

// Initialize the debug log viewer
void debuglog_init(void);

// Check if the debug log modal is currently visible
bool debuglog_is_visible(void);

// Show the debug log modal
void debuglog_show(void);

// Handle input while the debug log is visible (close, scroll, level changes)
void debuglog_update(void);

// Render the debug log modal
void debuglog_draw(void);

// Log subscriber callback — register with log_subscribe()
void debuglog_subscriber(LogLevel level, const char *message);

#endif // DEBUGLOG_H
