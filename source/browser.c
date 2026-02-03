/*
 * Folder browser - Navigate and select folders on SD card
 */

#include "browser.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_ENTRIES 256
#define MAX_PATH_LEN 256

typedef struct {
    char name[256];
    bool isDirectory;
} DirEntry;

static char currentPath[MAX_PATH_LEN];
static char selectedPath[MAX_PATH_LEN];
static DirEntry entries[MAX_ENTRIES];
static int entryCount = 0;
static int selectedIndex = 0;
static int scrollOffset = 0;
static bool cancelled = false;
static bool folderSelected = false;

static int compare_entries(const void *a, const void *b) {
    const DirEntry *ea = (const DirEntry *)a;
    const DirEntry *eb = (const DirEntry *)b;
    
    // Directories first
    if (ea->isDirectory && !eb->isDirectory) return -1;
    if (!ea->isDirectory && eb->isDirectory) return 1;
    
    // Then alphabetically
    return strcasecmp(ea->name, eb->name);
}

static void load_directory(const char *path) {
    entryCount = 0;
    selectedIndex = 0;
    scrollOffset = 0;
    
    strncpy(currentPath, path, MAX_PATH_LEN - 1);
    
    // Remove trailing slash if present (except for root)
    size_t len = strlen(currentPath);
    if (len > 1 && currentPath[len - 1] == '/') {
        currentPath[len - 1] = '\0';
    }
    
    DIR *dir = opendir(currentPath);
    if (!dir) {
        return;
    }
    
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL && entryCount < MAX_ENTRIES) {
        // Skip hidden files and . entry
        if (ent->d_name[0] == '.') {
            // Allow .. for going up
            if (strcmp(ent->d_name, "..") != 0) {
                continue;
            }
        }
        
        // Check if it's a directory
        char fullPath[MAX_PATH_LEN];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, ent->d_name);
        
        struct stat st;
        if (stat(fullPath, &st) != 0) {
            continue;
        }
        
        // Only show directories
        if (!S_ISDIR(st.st_mode)) {
            continue;
        }
        
        strncpy(entries[entryCount].name, ent->d_name, sizeof(entries[entryCount].name) - 1);
        entries[entryCount].isDirectory = true;
        entryCount++;
    }
    
    closedir(dir);
    
    // Sort entries
    if (entryCount > 0) {
        qsort(entries, entryCount, sizeof(DirEntry), compare_entries);
    }
}

void browser_init(const char *startPath) {
    cancelled = false;
    folderSelected = false;
    selectedPath[0] = '\0';
    
    if (startPath && startPath[0]) {
        load_directory(startPath);
    } else {
        load_directory("sdmc:/");
    }
}

void browser_exit(void) {
    entryCount = 0;
}

bool browser_update(u32 kDown) {
    if (folderSelected || cancelled) {
        return folderSelected;
    }
    
    // Navigation
    if (kDown & KEY_DOWN) {
        selectedIndex++;
        if (selectedIndex >= entryCount) {
            selectedIndex = 0;
            scrollOffset = 0;
        }
        if (selectedIndex >= scrollOffset + UI_VISIBLE_ITEMS) {
            scrollOffset = selectedIndex - UI_VISIBLE_ITEMS + 1;
        }
    }
    
    if (kDown & KEY_UP) {
        selectedIndex--;
        if (selectedIndex < 0) {
            selectedIndex = entryCount > 0 ? entryCount - 1 : 0;
            scrollOffset = entryCount > UI_VISIBLE_ITEMS ? entryCount - UI_VISIBLE_ITEMS : 0;
        }
        if (selectedIndex < scrollOffset) {
            scrollOffset = selectedIndex;
        }
    }
    
    // Page navigation
    if (kDown & KEY_R) {
        selectedIndex += UI_VISIBLE_ITEMS;
        if (selectedIndex >= entryCount) {
            selectedIndex = entryCount > 0 ? entryCount - 1 : 0;
        }
        if (selectedIndex >= scrollOffset + UI_VISIBLE_ITEMS) {
            scrollOffset = selectedIndex - UI_VISIBLE_ITEMS + 1;
        }
    }
    
    if (kDown & KEY_L) {
        selectedIndex -= UI_VISIBLE_ITEMS;
        if (selectedIndex < 0) {
            selectedIndex = 0;
        }
        if (selectedIndex < scrollOffset) {
            scrollOffset = selectedIndex;
        }
    }
    
    // Enter directory
    if ((kDown & KEY_A) && entryCount > 0) {
        if (strcmp(entries[selectedIndex].name, "..") == 0) {
            // Go up one level
            char *lastSlash = strrchr(currentPath, '/');
            if (lastSlash && lastSlash != currentPath) {
                *lastSlash = '\0';
                load_directory(currentPath);
            } else if (strcmp(currentPath, "sdmc:") != 0) {
                load_directory("sdmc:/");
            }
        } else {
            // Enter subdirectory
            char newPath[MAX_PATH_LEN];
            snprintf(newPath, sizeof(newPath), "%s/%s", currentPath, entries[selectedIndex].name);
            load_directory(newPath);
        }
    }
    
    // Select current folder
    if (kDown & KEY_X) {
        strncpy(selectedPath, currentPath, MAX_PATH_LEN - 1);
        folderSelected = true;
        return true;
    }
    
    // Create new folder
    if (kDown & KEY_Y) {
        char newFolderName[256] = "";
        if (ui_show_keyboard("New Folder Name", newFolderName, sizeof(newFolderName), false)) {
            if (newFolderName[0] != '\0') {
                char newPath[MAX_PATH_LEN];
                snprintf(newPath, sizeof(newPath), "%s/%s", currentPath, newFolderName);
                if (mkdir(newPath, 0755) == 0) {
                    load_directory(currentPath);  // Refresh to show new folder
                }
            }
        }
    }
    
    // Cancel
    if (kDown & KEY_B) {
        cancelled = true;
        return false;
    }
    
    return false;
}

bool browser_was_cancelled(void) {
    return cancelled;
}

const char *browser_get_selected_path(void) {
    return selectedPath;
}

void browser_draw(void) {
    ui_draw_header("Select ROM Folder");
    
    // Show current path
    float y = UI_HEADER_HEIGHT + UI_PADDING;
    ui_draw_text(UI_PADDING, y, currentPath, UI_COLOR_TEXT_DIM);
    y += UI_LINE_HEIGHT + UI_PADDING;
    
    float itemWidth = SCREEN_TOP_WIDTH - (UI_PADDING * 2);
    
    if (entryCount == 0) {
        ui_draw_text(UI_PADDING, y, "(empty folder)", UI_COLOR_TEXT_DIM);
    } else {
        // Draw visible items (fewer items due to path display)
        int visibleItems = UI_VISIBLE_ITEMS - 1;
        int visibleEnd = scrollOffset + visibleItems;
        if (visibleEnd > entryCount) visibleEnd = entryCount;
        
        for (int i = scrollOffset; i < visibleEnd; i++) {
            char displayName[260];
            snprintf(displayName, sizeof(displayName), "[%s]", entries[i].name);
            ui_draw_list_item(UI_PADDING, y, itemWidth, displayName, i == selectedIndex);
            y += UI_LINE_HEIGHT;
        }
    }
    
    // Help text
    ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING,
                 "A: Open | X: Select | Y: New Folder | B: Cancel", UI_COLOR_TEXT_DIM);
}
