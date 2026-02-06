/*
 * Folder browser - Navigate and select folders on SD card
 */

#include "browser.h"
#include "ui.h"
#include "listnav.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <citro2d.h>

#define MAX_ENTRIES 256
#define MAX_PATH_LEN 256
#define MAX_NAME_LEN 256
#define PATH_BUFFER_LEN 512 // Extra space for path concatenation
#define FOLDER_ICON_SIZE 16
#define FOLDER_ICON_COLOR 0xFFE0A040 // Amber/orange folder color

typedef struct {
    char name[MAX_NAME_LEN];
    bool isDirectory;
} DirEntry;

static char currentPath[MAX_PATH_LEN];
static char selectedPath[PATH_BUFFER_LEN];      // Needs room for path + "/" + name
static char rootPath[MAX_PATH_LEN];             // If set, can't navigate above this
static char defaultNewFolderName[MAX_NAME_LEN]; // Default name for new folder
static DirEntry entries[MAX_ENTRIES];
static ListNav nav;
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
    listnav_set(&nav, 0, 0);
    nav.visibleItems = UI_VISIBLE_ITEMS - 1; // path display takes one line

    snprintf(currentPath, sizeof(currentPath), "%s", path);

    // Remove trailing slash if present (except for root)
    size_t len = strlen(currentPath);
    if (len > 1 && currentPath[len - 1] == '/') {
        currentPath[len - 1] = '\0';
    }

    DIR *dir = opendir(currentPath);
    if (!dir) {
        return;
    }

    // Add ".." entry explicitly if not at root (some filesystems don't return it)
    bool atRoot = (isRooted && strcmp(currentPath, rootPath) == 0) ||
                  (!isRooted && (strcmp(currentPath, "sdmc:") == 0 || strcmp(currentPath, "sdmc:/") == 0));
    if (!atRoot) {
        snprintf(entries[nav.count].name, MAX_NAME_LEN, "..");
        entries[nav.count].isDirectory = true;
        nav.count++;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL && nav.count < MAX_ENTRIES) {
        // Skip hidden files including . and .. (we added .. manually above)
        if (ent->d_name[0] == '.') {
            continue;
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

        snprintf(entries[nav.count].name, MAX_NAME_LEN, "%s", ent->d_name);
        entries[nav.count].isDirectory = true;
        nav.count++;
    }

    closedir(dir);

    // Sort entries
    if (nav.count > 0) {
        qsort(entries, nav.count, sizeof(DirEntry), compare_entries);
    }
    nav.total = nav.count;
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
    nav.count = 0;
    nav.total = 0;
}

bool browser_update(u32 kDown) {
    if (folderSelected || cancelled) {
        return folderSelected;
    }

    // Navigation (D-pad and L/R paging)
    listnav_update(&nav, kDown);

    // Enter directory
    if ((kDown & KEY_A) && nav.count > 0) {
        if (strcmp(entries[nav.selectedIndex].name, "..") == 0) {
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
            snprintf(newPath, sizeof(newPath), "%s/%s", currentPath, entries[nav.selectedIndex].name);
            load_directory(newPath);
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

bool browser_select_current(void) {
    if (nav.count == 0) return false;
    if (strcmp(entries[nav.selectedIndex].name, "..") == 0) return false;
    snprintf(selectedPath, sizeof(selectedPath), "%s/%s", currentPath, entries[nav.selectedIndex].name);
    folderSelected = true;
    return true;
}

bool browser_create_folder(void) {
    char newFolderName[MAX_NAME_LEN];
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
                load_directory(currentPath);
                // Find and select the newly created folder
                for (int i = 0; i < nav.count; i++) {
                    if (strcmp(entries[i].name, newFolderName) == 0) {
                        nav.selectedIndex = i;
                        int vis = nav.visibleItems > 0 ? nav.visibleItems : UI_VISIBLE_ITEMS;
                        if (nav.selectedIndex >= nav.scrollOffset + vis) {
                            nav.scrollOffset = nav.selectedIndex - vis + 1;
                        }
                        break;
                    }
                }
                return true;
            }
        }
    }
    return false;
}

const char *browser_get_current_name(void) {
    if (nav.count == 0) return "";
    if (strcmp(entries[nav.selectedIndex].name, "..") == 0) return "";
    return entries[nav.selectedIndex].name;
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
    float iconOffset = FOLDER_ICON_SIZE + 8; // Icon width plus spacing

    if (nav.count == 0) {
        ui_draw_text(UI_PADDING, y, "(empty folder)", UI_COLOR_TEXT_DIM);
    } else {
        int start, end;
        listnav_visible_range(&nav, &start, &end);

        for (int i = start; i < end; i++) {
            if (i == nav.selectedIndex) {
                ui_draw_rect(UI_PADDING, y, itemWidth, UI_LINE_HEIGHT, UI_COLOR_SELECTED);
            }

            draw_folder_icon(UI_PADDING + 2, y + 1, FOLDER_ICON_SIZE, FOLDER_ICON_COLOR);
            ui_draw_text(UI_PADDING + iconOffset, y + 2, entries[i].name, UI_COLOR_TEXT);

            y += UI_LINE_HEIGHT;
        }
    }

    // Help text
    ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING, "A: Open | B: Cancel", UI_COLOR_TEXT_DIM);
}
