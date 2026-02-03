/*
 * Folder browser - Navigate and select folders on SD card
 */

#ifndef BROWSER_H
#define BROWSER_H

#include <stdbool.h>
#include <3ds.h>

// Initialize the browser at a starting path
void browser_init(const char *startPath);

// Cleanup browser resources
void browser_exit(void);

// Update browser state, returns true if a folder was selected
bool browser_update(u32 kDown);

// Check if browser was cancelled
bool browser_was_cancelled(void);

// Get the selected folder path
const char *browser_get_selected_path(void);

// Draw the browser UI
void browser_draw(void);

#endif // BROWSER_H
