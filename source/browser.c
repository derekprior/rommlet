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
#include <citro2d.h>

#define MAX_ENTRIES 256
#define MAX_PATH_LEN 256
#define MAX_NAME_LEN 256
#define PATH_BUFFER_LEN 512  // Extra space for path concatenation
#define FOLDER_ICON_SIZE 16
#define FOLDER_ICON_COLOR 0xFFE0A040  // Amber/orange folder color

typedef struct {
    char name[MAX_NAME_LEN];
    bool isDirectory;
} DirEntry;

static char currentPath[MAX_PATH_LEN];
static char selectedPath[PATH_BUFFER_LEN];  // Needs room for path + "/" + name
static char rootPath[MAX_PATH_LEN];  // If set, can't navigate above this
static char defaultNewFolderName[MAX_NAME_LEN];  // Default name for new folder
static DirEntry entries[MAX_ENTRIES];
static int entryCount = 0;
static int selectedIndex = 0;
static int scrollOffset = 0;
static bool cancelled = false;
static bool folderSelected = false;
static bool isRooted = false;

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
            // Allow .. for going up (unless rooted and at root)
            if (strcmp(ent->d_name, "..") != 0) {
                continue;
            }
            // Skip .. if we're at the root in rooted mode
            if (isRooted && strcmp(currentPath, rootPath) == 0) {
                continue;
            }
        }
        
        // Check if it's a directory
        char fullPath[PATH_BUFFER_LEN];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, ent->d_name);
        
        struct stat st;
        if (stat(fullPath, &st) != 0) {
            continue;
        }
        
        // Only show directories
        if (!S_ISDIR(st.st_mode)) {
            continue;
        }
        
        snprintf(entries[entryCount].name, MAX_NAME_LEN, "%s", ent->d_name);
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
    rootPath[0] = '\0';
    defaultNewFolderName[0] = '\0';
    isRooted = false;
    
    if (startPath && startPath[0]) {
        load_directory(startPath);
    } else {
        load_directory("sdmc:/");
    }
}

void browser_init_rooted(const char *root, const char *defaultNewFolder) {
    cancelled = false;
    folderSelected = false;
    selectedPath[0] = '\0';
    isRooted = true;
    
    snprintf(rootPath, sizeof(rootPath), "%s", root);
    
    if (defaultNewFolder && defaultNewFolder[0]) {
        snprintf(defaultNewFolderName, sizeof(defaultNewFolderName), "%s", defaultNewFolder);
    } else {
        defaultNewFolderName[0] = '\0';
    }
    
    load_directory(root);
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
            // Go up one level - but respect root if rooted
            if (isRooted && strcmp(currentPath, rootPath) == 0) {
                // Already at root, can't go up
            } else {
                char *lastSlash = strrchr(currentPath, '/');
                if (lastSlash && lastSlash != currentPath) {
                    *lastSlash = '\0';
                    load_directory(currentPath);
                } else if (strcmp(currentPath, "sdmc:") != 0) {
                    load_directory("sdmc:/");
                }
            }
        } else {
            // Enter subdirectory
            char newPath[PATH_BUFFER_LEN];
            snprintf(newPath, sizeof(newPath), "%s/%s", currentPath, entries[selectedIndex].name);
            load_directory(newPath);
        }
    }
    
    // Select highlighted folder
    if ((kDown & KEY_X) && entryCount > 0) {
        // Don't allow selecting ".." - that would be confusing
        if (strcmp(entries[selectedIndex].name, "..") == 0) {
            // Go up instead of selecting parent
        } else {
            snprintf(selectedPath, sizeof(selectedPath), "%s/%s", currentPath, entries[selectedIndex].name);
            folderSelected = true;
            return true;
        }
    }
    
    // Create new folder
    if (kDown & KEY_Y) {
        char newFolderName[MAX_NAME_LEN];
        // Use default folder name if set, otherwise empty
        if (defaultNewFolderName[0]) {
            snprintf(newFolderName, sizeof(newFolderName), "%s", defaultNewFolderName);
        } else {
            newFolderName[0] = '\0';
        }
        if (ui_show_keyboard("New Folder Name", newFolderName, sizeof(newFolderName), false)) {
            if (newFolderName[0] != '\0') {
                char newPath[PATH_BUFFER_LEN];
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

const char *browser_get_selected_folder_name(void) {
    // Return just the last component of the selected path
    const char *lastSlash = strrchr(selectedPath, '/');
    if (lastSlash) {
        return lastSlash + 1;
    }
    return selectedPath;
}

// Draw a folder icon at the given position
static void draw_folder_icon(float x, float y, float size, u32 color) {
    float scale = size / 16.0f;
    
    // Main folder body (rectangle)
    float bodyX = x;
    float bodyY = y + 3 * scale;
    float bodyW = 14 * scale;
    float bodyH = 10 * scale;
    C2D_DrawRectSolid(bodyX, bodyY, 0, bodyW, bodyH, color);
    
    // Tab on top left
    float tabW = 5 * scale;
    float tabH = 3 * scale;
    C2D_DrawRectSolid(x, y, 0, tabW, tabH, color);
}

void browser_draw(void) {
    ui_draw_header("Select ROM Folder");
    
    // Show current path
    float y = UI_HEADER_HEIGHT + UI_PADDING;
    ui_draw_text(UI_PADDING, y, currentPath, UI_COLOR_TEXT_DIM);
    y += UI_LINE_HEIGHT + UI_PADDING;
    
    float itemWidth = SCREEN_TOP_WIDTH - (UI_PADDING * 2);
    float iconOffset = FOLDER_ICON_SIZE + 4;  // Icon width plus spacing
    
    if (entryCount == 0) {
        ui_draw_text(UI_PADDING, y, "(empty folder)", UI_COLOR_TEXT_DIM);
    } else {
        // Draw visible items (fewer items due to path display)
        int visibleItems = UI_VISIBLE_ITEMS - 1;
        int visibleEnd = scrollOffset + visibleItems;
        if (visibleEnd > entryCount) visibleEnd = entryCount;
        
        for (int i = scrollOffset; i < visibleEnd; i++) {
            // Draw selection background if selected
            if (i == selectedIndex) {
                ui_draw_rect(UI_PADDING, y, itemWidth, UI_LINE_HEIGHT, UI_COLOR_SELECTED);
            }
            
            // Draw folder icon
            draw_folder_icon(UI_PADDING + 2, y + 1, FOLDER_ICON_SIZE, FOLDER_ICON_COLOR);
            
            // Draw folder name
            ui_draw_text(UI_PADDING + iconOffset, y + 2, entries[i].name,
                        i == selectedIndex ? UI_COLOR_TEXT : UI_COLOR_TEXT);
            
            y += UI_LINE_HEIGHT;
        }
    }
    
    // Help text
    ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING,
                 "A: Open | X: Select | Y: New Folder | B: Cancel", UI_COLOR_TEXT_DIM);
}
