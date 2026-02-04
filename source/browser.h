/*
 * Folder browser - Navigate and select folders on SD card
 */

#ifndef BROWSER_H
#define BROWSER_H

#include <stdbool.h>
#include <3ds.h>

// Initialize the browser at a starting path
void browser_init(const char *startPath);

// Initialize browser rooted in a specific directory (can't navigate above it)
// defaultNewFolder: if provided, used as default name when creating new folder
void browser_init_rooted(const char *rootPath, const char *defaultNewFolder);

// Cleanup browser resources
void browser_exit(void);

// Update browser state, returns true if a folder was selected
bool browser_update(u32 kDown);

// Check if browser was cancelled
bool browser_was_cancelled(void);

// Get the selected folder path
const char *browser_get_selected_path(void);

// Get just the folder name (last component of selected path)
const char *browser_get_selected_folder_name(void);

// Draw the browser UI
void browser_draw(void);

#endif // BROWSER_H
