/*
 * Rommlet - RomM Client for Nintendo 3DS
 * A homebrew app to browse ROMs on a RomM server
 *
 * MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <3ds.h>
#include <citro2d.h>
#include "config.h"
#include "api.h"
#include "log.h"
#include "ui.h"
#include "browser.h"
#include "sound.h"
#include "screens/settings.h"
#include "screens/platforms.h"
#include "screens/roms.h"
#include "screens/romdetail.h"
#include "screens/bottom.h"
#include "queue.h"
#include "screens/queuescreen.h"
#include "screens/search.h"
#include "screens/about.h"
#include "debuglog.h"
#include "zip.h"

// App states
typedef enum {
    STATE_LOADING,
    STATE_SETTINGS,
    STATE_PLATFORMS,
    STATE_ROMS,
    STATE_ROM_DETAIL,
    STATE_SELECT_ROM_FOLDER,
    STATE_QUEUE,
    STATE_SEARCH_FORM,
    STATE_SEARCH_RESULTS,
    STATE_ABOUT
} AppState;

static AppState currentState = STATE_LOADING;
static bool needsConfigSetup = false;
static bool queueAddPending = false;   // Track if folder selection is for queue add
static bool queueConfirmShown = false; // Track if clear-queue confirmation is showing

// Navigation stack — push current state before entering a new screen, pop on back
#define NAV_STACK_MAX 8
static AppState navStack[NAV_STACK_MAX];
static int navStackSize = 0;

static void nav_push(AppState state) {
    if (navStackSize < NAV_STACK_MAX) {
        navStack[navStackSize++] = state;
    }
}

static AppState nav_pop(void) {
    if (navStackSize > 0) {
        return navStack[--navStackSize];
    }
    return STATE_PLATFORMS;
}

static void nav_clear(void) {
    navStackSize = 0;
}

// Shared state
static Config config;
static Platform *platforms = NULL;
static int platformCount = 0;
static int selectedPlatformIndex = 0;
static RomDetail *romDetail = NULL;
static int lastRomListIndex = -1;                     // Track selection changes in ROM list
static int lastSearchListIndex = -1;                  // Track selection changes in search results
static char currentPlatformSlug[CONFIG_MAX_SLUG_LEN]; // For folder mapping
static AppState folderPickerReturnState;              // Modal return state for folder picker

// Render target (needed for loading screen)
static C3D_RenderTarget *topScreen = NULL;

// Show loading indicator and render a frame
static void show_loading(const char *message) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(topScreen, UI_COLOR_BG);
    C2D_SceneBegin(topScreen);
    ui_draw_loading(message);
    bottom_draw();
    C3D_FrameEnd(0);
}

// Download context for progress callback
static char downloadNameBuf[384];
static const char *downloadName = NULL;
static const char *downloadQueueText = NULL;

static void set_download_name(const char *slug, const char *name) {
    snprintf(downloadNameBuf, sizeof(downloadNameBuf), "[%s] %s", slug, name);
    downloadName = downloadNameBuf;
}

// Download progress callback - renders progress bar each chunk
// Returns true to continue, false to cancel
static bool download_progress(u32 downloaded, u32 total) {
    float progress = total > 0 ? (float)downloaded / total : -1.0f;

    char sizeText[64];
    if (total > 0) {
        snprintf(sizeText, sizeof(sizeText), "%.1f / %.1f MB", downloaded / (1024.0f * 1024.0f),
                 total / (1024.0f * 1024.0f));
    } else {
        snprintf(sizeText, sizeof(sizeText), "%.1f MB downloaded", downloaded / (1024.0f * 1024.0f));
    }

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(topScreen, UI_COLOR_BG);
    C2D_SceneBegin(topScreen);
    ui_draw_progress(progress, "Downloading...", sizeText, downloadName, downloadQueueText);
    bottom_draw();
    C3D_FrameEnd(0);

    return !bottom_check_cancel();
}

// Extraction progress callback - renders progress bar each chunk
static bool extract_progress(uint32_t extracted, uint32_t total) {
    float progress = total > 0 ? (float)extracted / total : -1.0f;

    char sizeText[64];
    if (total > 0) {
        snprintf(sizeText, sizeof(sizeText), "%.1f / %.1f MB", extracted / (1024.0f * 1024.0f),
                 total / (1024.0f * 1024.0f));
    } else {
        snprintf(sizeText, sizeof(sizeText), "%.1f MB extracted", extracted / (1024.0f * 1024.0f));
    }

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(topScreen, UI_COLOR_BG);
    C2D_SceneBegin(topScreen);
    ui_draw_progress(progress, "Extracting...", sizeText, downloadName, downloadQueueText);
    bottom_draw();
    C3D_FrameEnd(0);

    return !bottom_check_cancel();
}

// Helper to fetch platforms from API
static void fetch_platforms(void) {
    show_loading("Fetching platforms...");
    log_info("Fetching platforms...");
    if (platforms) {
        api_free_platforms(platforms, platformCount);
        platforms = NULL;
    }
    platforms = api_get_platforms(&platformCount);
    if (platforms) {
        log_info("Found %d platforms", platformCount);
        platforms_set_data(platforms, platformCount);
    } else {
        log_error("Failed to fetch platforms");
    }
}

// Build full destination path for a ROM file
static void build_rom_path(char *dest, size_t destSize, const char *folderName, const char *fsName) {
    snprintf(dest, destSize, "%s/%s/%s", config.romFolder, folderName, fsName);
}

// Check if a ROM file exists on disk by platform slug and filename.
// For zip files, checks if any extracted file with the same stem exists.
static bool check_file_exists(const char *platformSlug, const char *fileName) {
    const char *folderName = config_get_platform_folder(platformSlug);
    if (!folderName || !folderName[0]) return false;
    char path[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 256 + 3];
    build_rom_path(path, sizeof(path), folderName, fileName);

    // Exact match first
    struct stat st;
    if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) return true;

    // For zip files, check for extracted files matching the stem
    if (!zip_is_zip_file(fileName)) return false;

    // Get the stem (filename without .zip extension)
    char stem[256];
    snprintf(stem, sizeof(stem), "%s", fileName);
    char *dot = strrchr(stem, '.');
    if (dot) *dot = '\0';
    size_t stemLen = strlen(stem);

    // Scan the directory for files starting with the stem
    char dirPath[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 2];
    snprintf(dirPath, sizeof(dirPath), "%s/%s", config.romFolder, folderName);
    DIR *dir = opendir(dirPath);
    if (!dir) return false;

    struct dirent *entry;
    bool found = false;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, stem, stemLen) == 0 && entry->d_name[stemLen] == '.') {
            char fullPath[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 256 + 3];
            snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, entry->d_name);
            if (stat(fullPath, &st) == 0 && S_ISREG(st.st_mode)) {
                found = true;
                break;
            }
        }
    }
    closedir(dir);
    return found;
}

// Check if a platform folder is configured and valid on disk
static bool check_platform_folder_valid(const char *platformSlug) {
    const char *folderName = config_get_platform_folder(platformSlug);
    if (!folderName || !folderName[0]) {
        // Auto-map if a folder matching the slug exists
        char autoPath[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 2];
        snprintf(autoPath, sizeof(autoPath), "%s/%s", config.romFolder, platformSlug);
        struct stat autoSt;
        if (stat(autoPath, &autoSt) == 0 && S_ISDIR(autoSt.st_mode)) {
            config_set_platform_folder(&config, platformSlug, platformSlug);
            log_info("Auto-mapped platform '%s' to existing folder", platformSlug);
            folderName = platformSlug;
        } else {
            log_info("No folder configured for platform '%s'", platformSlug);
            return false;
        }
    }
    char folderPath[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 2];
    snprintf(folderPath, sizeof(folderPath), "%s/%s", config.romFolder, folderName);
    struct stat st;
    if (!(stat(folderPath, &st) == 0 && S_ISDIR(st.st_mode))) {
        log_info("Folder '%s' no longer exists, select a new one", folderName);
        return false;
    }
    return true;
}

// Get the currently focused ROM across any rom-list state, setting platform slug.
// For STATE_ROM_DETAIL uses romDetail fields; for list states uses the selected list item.
// Returns false if no ROM is focused. Copies data into *out and sets *slug.
static bool get_focused_rom(Rom *out, const char **slug) {
    if (currentState == STATE_ROM_DETAIL && romDetail) {
        out->id = romDetail->id;
        out->platformId = romDetail->platformId;
        snprintf(out->name, sizeof(out->name), "%s", romDetail->name);
        snprintf(out->fsName, sizeof(out->fsName), "%s", romDetail->fsName);
        *slug = currentPlatformSlug;
        return true;
    } else if (currentState == STATE_ROMS) {
        const Rom *rom = roms_get_at(roms_get_selected_index());
        if (!rom) return false;
        *out = *rom;
        *slug = currentPlatformSlug;
        return true;
    } else if (currentState == STATE_SEARCH_RESULTS) {
        const Rom *rom = search_get_result_at(search_get_selected_index());
        if (!rom) return false;
        *out = *rom;
        const char *searchSlug = search_get_platform_slug(rom->platformId);
        snprintf(currentPlatformSlug, sizeof(currentPlatformSlug), "%s", searchSlug);
        *slug = currentPlatformSlug;
        return true;
    }
    *slug = currentPlatformSlug;
    return false;
}

// Transition to targetState and sync the bottom screen for ROM actions
static void sync_bottom_after_action(AppState targetState) {
    currentState = targetState;
    bottom_set_mode(BOTTOM_MODE_ROM_ACTIONS);
    bottom_set_queue_count(queue_count());
    if (targetState == STATE_ROMS) {
        const Rom *rom = roms_get_at(roms_get_selected_index());
        if (rom) {
            bottom_set_rom_exists(check_file_exists(currentPlatformSlug, rom->fsName));
            bottom_set_rom_queued(queue_contains(rom->id));
        }
        lastRomListIndex = roms_get_selected_index();
    } else if (targetState == STATE_SEARCH_RESULTS) {
        const Rom *rom = search_get_result_at(search_get_selected_index());
        if (rom) {
            const char *slug = search_get_platform_slug(rom->platformId);
            bottom_set_rom_exists(check_file_exists(slug, rom->fsName));
            bottom_set_rom_queued(queue_contains(rom->id));
        }
    } else if (targetState == STATE_ROM_DETAIL && romDetail) {
        bottom_set_rom_exists(check_file_exists(currentPlatformSlug, romDetail->fsName));
        bottom_set_rom_queued(queue_contains(romDetail->id));
    }
}

// Update bottom screen state for the selected ROM in the list
static void sync_roms_bottom(int index) {
    const Rom *rom = roms_get_at(index);
    if (rom) {
        bottom_set_rom_exists(check_file_exists(currentPlatformSlug, rom->fsName));
        bottom_set_rom_queued(queue_contains(rom->id));
    }
    lastRomListIndex = index;
}

// Extract a zip file after download, showing progress. Returns true on success.
static bool extract_if_zip(const char *destPath, const char *destDir) {
    // Get just the filename from the path
    const char *filename = strrchr(destPath, '/');
    filename = filename ? filename + 1 : destPath;

    if (!zip_is_zip_file(filename)) return true;

    log_info("Extracting zip: %s", destPath);
    if (zip_extract(destPath, destDir, extract_progress)) {
        log_info("Extraction complete!");
        return true;
    } else {
        log_error("Extraction failed!");
        return false;
    }
}

// Download the currently focused ROM to the given platform folder
static void download_focused_rom(const Rom *rom, const char *slug, const char *folderName) {
    char destPath[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 256 + 3];
    build_rom_path(destPath, sizeof(destPath), folderName, rom->fsName);
    bottom_set_mode(BOTTOM_MODE_DOWNLOADING);
    set_download_name(slug, rom->name);
    downloadQueueText = NULL;
    log_info("Downloading to: %s", destPath);
    if (api_download_rom(rom->id, rom->fsName, destPath, download_progress)) {
        log_info("Download complete!");
        char destDir[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 2];
        snprintf(destDir, sizeof(destDir), "%s/%s", config.romFolder, folderName);
        if (!extract_if_zip(destPath, destDir)) {
            remove(destPath);
        }
    } else {
        log_error("Download failed!");
    }
}

// Download a single queue entry. Returns true on success.
static bool download_queue_entry(QueueEntry *entry) {
    const char *folderName = config_get_platform_folder(entry->platformSlug);
    if (!folderName || !folderName[0]) {
        log_error("No folder for platform '%s', skipping", entry->platformSlug);
        return false;
    }
    char destPath[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 256 + 3];
    build_rom_path(destPath, sizeof(destPath), folderName, entry->fsName);
    log_info("Downloading '%s' to: %s", entry->name, destPath);
    if (!api_download_rom(entry->romId, entry->fsName, destPath, download_progress)) return false;
    char destDir[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 2];
    snprintf(destDir, sizeof(destDir), "%s/%s", config.romFolder, folderName);
    if (!extract_if_zip(destPath, destDir)) {
        remove(destPath);
        return false;
    }
    return true;
}

// Fetch and display ROM detail, updating all navigation state
static bool open_rom_detail(int romId, const char *slug) {
    show_loading("Loading ROM details...");
    log_info("Fetching ROM details for ID %d...", romId);
    if (romDetail) {
        api_free_rom_detail(romDetail);
        romDetail = NULL;
    }
    romDetail = api_get_rom_detail(romId);
    if (!romDetail) {
        log_error("Failed to fetch ROM details");
        return false;
    }
    romdetail_set_data(romDetail);
    snprintf(currentPlatformSlug, sizeof(currentPlatformSlug), "%s", slug);
    nav_push(currentState);
    bottom_set_mode(BOTTOM_MODE_ROM_ACTIONS);
    bottom_set_rom_exists(check_file_exists(currentPlatformSlug, romDetail->fsName));
    bottom_set_rom_queued(queue_contains(romDetail->id));
    bottom_set_queue_count(queue_count());
    currentState = STATE_ROM_DETAIL;
    return true;
}

// Execute search and transition to results
static void execute_search(void) {
    const char *term = search_get_term();
    if (!term || !term[0]) return;

    show_loading("Searching...");
    int idCount;
    const int *ids = search_get_platform_ids(&idCount);
    int resultCount, resultTotal;
    Rom *results = api_search_roms(term, ids, idCount, 0, ROM_PAGE_SIZE, &resultCount, &resultTotal);
    if (results) {
        log_info("Search found %d/%d results", resultCount, resultTotal);
        search_set_results(results, resultCount, resultTotal);
    } else {
        log_info("Search returned no results");
        search_set_results(NULL, 0, 0);
    }
    nav_push(currentState);
    bottom_set_mode(BOTTOM_MODE_ROM_ACTIONS);
    bottom_set_queue_count(queue_count());
    const Rom *firstRom = search_get_result_at(0);
    if (firstRom) {
        const char *slug = search_get_platform_slug(firstRom->platformId);
        bottom_set_rom_exists(check_file_exists(slug, firstRom->fsName));
        bottom_set_rom_queued(queue_contains(firstRom->id));
    }
    currentState = STATE_SEARCH_RESULTS;
    lastSearchListIndex = 0;
}

// ---------------------------------------------------------------------------
// Bottom action dispatch
// ---------------------------------------------------------------------------

static void handle_bottom_action(BottomAction action) {
    if (action == BOTTOM_ACTION_NONE) return;

    // Settings actions
    if (action == BOTTOM_ACTION_SAVE_SETTINGS && currentState == STATE_SETTINGS) {
        sound_play_click();
        config_save(&config);
        api_set_auth(config.username, config.password);
        api_set_base_url(config.serverUrl);
        bottom_set_mode(BOTTOM_MODE_DEFAULT);
        nav_clear();
        currentState = STATE_PLATFORMS;
        fetch_platforms();
        return;
    }
    if (action == BOTTOM_ACTION_CANCEL_SETTINGS && currentState == STATE_SETTINGS) {
        sound_play_pop();
        bottom_set_mode(BOTTOM_MODE_DEFAULT);
        currentState = nav_pop();
        return;
    }

    // Toolbar navigation
    if (action == BOTTOM_ACTION_OPEN_SETTINGS && currentState != STATE_SETTINGS) {
        sound_play_click();
        nav_push(currentState);
        bottom_set_settings_mode(config_is_valid(&config));
        currentState = STATE_SETTINGS;
        return;
    }
    if (action == BOTTOM_ACTION_GO_HOME && currentState != STATE_PLATFORMS) {
        sound_play_pop();
        bottom_set_mode(BOTTOM_MODE_DEFAULT);
        lastRomListIndex = -1;
        lastSearchListIndex = -1;
        queueAddPending = false;
        nav_clear();
        currentState = STATE_PLATFORMS;
        return;
    }
    if (action == BOTTOM_ACTION_OPEN_QUEUE && currentState != STATE_QUEUE) {
        sound_play_click();
        nav_push(currentState);
        queue_clear_failed();
        queue_screen_init();
        bottom_set_mode(BOTTOM_MODE_QUEUE);
        bottom_set_queue_count(queue_count());
        currentState = STATE_QUEUE;
        return;
    }
    if (action == BOTTOM_ACTION_OPEN_SEARCH) {
        sound_play_click();
        if (currentState == STATE_SEARCH_RESULTS) {
            nav_pop(); // discard SearchForm entry; we're returning to it
        } else if (currentState != STATE_SEARCH_FORM) {
            nav_push(currentState);
        }
        const char *existingTerm = search_get_term();
        bool hasTerm = existingTerm && existingTerm[0];
        if (!hasTerm) {
            search_init(platforms, platformCount);
        }
        bottom_set_mode(BOTTOM_MODE_SEARCH_FORM);
        currentState = STATE_SEARCH_FORM;
        if (!hasTerm) {
            search_open_keyboard();
        }
        return;
    }
    if (action == BOTTOM_ACTION_OPEN_ABOUT && currentState != STATE_ABOUT) {
        sound_play_click();
        nav_push(currentState);
        bottom_set_mode(BOTTOM_MODE_ABOUT);
        currentState = STATE_ABOUT;
        return;
    }

    // ROM actions (download/queue) — valid in any rom-focused state
    bool romFocused =
        (currentState == STATE_ROM_DETAIL || currentState == STATE_ROMS || currentState == STATE_SEARCH_RESULTS);

    if (action == BOTTOM_ACTION_DOWNLOAD_ROM && romFocused) {
        sound_play_click();
        const char *slug;
        Rom rom;
        if (get_focused_rom(&rom, &slug)) {
            queueAddPending = false;
            if (check_platform_folder_valid(slug)) {
                download_focused_rom(&rom, slug, config_get_platform_folder(slug));
                sync_bottom_after_action(currentState);
            } else {
                browser_init_rooted(config.romFolder, slug);
                bottom_set_mode(BOTTOM_MODE_FOLDER_BROWSER);
                folderPickerReturnState = currentState;
                currentState = STATE_SELECT_ROM_FOLDER;
            }
        }
        return;
    }
    if (action == BOTTOM_ACTION_QUEUE_ROM && romFocused) {
        sound_play_click();
        const char *slug;
        Rom rom;
        if (get_focused_rom(&rom, &slug)) {
            if (queue_contains(rom.id)) {
                queue_remove(rom.id);
                log_info("Removed '%s' from download queue", rom.name);
                bottom_set_rom_queued(false);
                bottom_set_queue_count(queue_count());
            } else {
                if (check_platform_folder_valid(slug)) {
                    if (queue_add(rom.id, rom.platformId, rom.name, rom.fsName, slug)) {
                        log_info("Added '%s' to download queue", rom.name);
                    }
                    bottom_set_rom_queued(queue_contains(rom.id));
                    bottom_set_queue_count(queue_count());
                } else {
                    queueAddPending = true;
                    browser_init_rooted(config.romFolder, slug);
                    bottom_set_mode(BOTTOM_MODE_FOLDER_BROWSER);
                    folderPickerReturnState = currentState;
                    currentState = STATE_SELECT_ROM_FOLDER;
                }
            }
        }
        return;
    }

    // Search actions
    if (action == BOTTOM_ACTION_SEARCH_FIELD && currentState == STATE_SEARCH_FORM) {
        search_open_keyboard();
        return;
    }
    if (action == BOTTOM_ACTION_SEARCH_EXECUTE && currentState == STATE_SEARCH_FORM) {
        sound_play_click();
        execute_search();
        return;
    }

    // Queue management actions
    if (action == BOTTOM_ACTION_START_DOWNLOADS && currentState == STATE_QUEUE) {
        sound_play_click();
        int count = queue_count();
        if (count > 0) {
            bottom_set_mode(BOTTOM_MODE_DOWNLOADING);
            int completed = 0;
            int i = 0;
            while (i < queue_count()) {
                QueueEntry *entry = queue_get(i);
                if (!entry) {
                    i++;
                    continue;
                }

                char queueText[64];
                snprintf(queueText, sizeof(queueText), "ROM %d of %d in your queue", completed + 1, count);
                set_download_name(entry->platformSlug, entry->name);
                downloadQueueText = queueText;

                if (download_queue_entry(entry)) {
                    log_info("Queue download complete: %s", entry->name);
                    queue_remove(entry->romId);
                    completed++;
                } else {
                    log_error("Queue download failed: %s", entry->name);
                    queue_set_failed(i, true);
                    i++;
                }
            }
            downloadName = NULL;
            downloadQueueText = NULL;
            bottom_set_mode(BOTTOM_MODE_QUEUE);
            bottom_set_queue_count(queue_count());
            queue_screen_init();
        }
        return;
    }
    if (action == BOTTOM_ACTION_CLEAR_QUEUE && currentState == STATE_QUEUE) {
        sound_play_click();
        if (queueConfirmShown) {
            queueConfirmShown = false;
            queue_clear();
            log_info("Download queue cleared");
            bottom_set_mode(BOTTOM_MODE_QUEUE);
            bottom_set_queue_count(0);
            queue_screen_init();
        } else {
            queueConfirmShown = true;
            bottom_set_mode(BOTTOM_MODE_QUEUE_CONFIRM);
        }
        return;
    }
    if (action == BOTTOM_ACTION_CANCEL_CLEAR && currentState == STATE_QUEUE) {
        sound_play_pop();
        queueConfirmShown = false;
        bottom_set_mode(BOTTOM_MODE_QUEUE);
        return;
    }
}

// ---------------------------------------------------------------------------
// Per-state update handlers
// ---------------------------------------------------------------------------

static void handle_state_loading(void) {
    if (needsConfigSetup) {
        currentState = STATE_SETTINGS;
        bottom_set_settings_mode(false);
    } else {
        currentState = STATE_PLATFORMS;
        bottom_set_mode(BOTTOM_MODE_DEFAULT);
        fetch_platforms();
    }
}

static void handle_state_settings(u32 kDown) {
    SettingsResult result = settings_update(kDown);
    if (result == SETTINGS_CANCELLED) {
        if (config_is_valid(&config)) {
            sound_play_pop();
            bottom_set_mode(BOTTOM_MODE_DEFAULT);
            currentState = nav_pop();
        } else {
            log_warn("Configuration not valid. Please complete all fields.");
        }
    }
}

static void handle_state_platforms(u32 kDown) {
    PlatformsResult result = platforms_update(kDown, &selectedPlatformIndex);
    if (result == PLATFORMS_SELECTED && platforms && selectedPlatformIndex < platformCount) {
        sound_play_click();
        show_loading("Fetching ROMs...");
        log_info("Fetching ROMs for %s...", platforms[selectedPlatformIndex].displayName);
        roms_clear();
        int romCount, romTotal;
        Rom *roms = api_get_roms(platforms[selectedPlatformIndex].id, 0, ROM_PAGE_SIZE, &romCount, &romTotal);
        if (roms) {
            log_info("Found %d/%d ROMs", romCount, romTotal);
            roms_set_data(roms, romCount, romTotal, platforms[selectedPlatformIndex].displayName);
            snprintf(currentPlatformSlug, sizeof(currentPlatformSlug), "%s", platforms[selectedPlatformIndex].slug);
            lastRomListIndex = -1;
            bottom_set_mode(BOTTOM_MODE_ROM_ACTIONS);
            bottom_set_queue_count(queue_count());
            sync_roms_bottom(0);
            nav_push(currentState);
            currentState = STATE_ROMS;
        } else {
            log_error("Failed to fetch ROMs");
        }
    }
}

static void handle_state_roms(u32 kDown) {
    RomsResult result = roms_update(kDown);

    int curIdx = roms_get_selected_index();
    if (curIdx != lastRomListIndex && curIdx < roms_get_count()) {
        sync_roms_bottom(curIdx);
    }

    if (result == ROMS_BACK) {
        sound_play_pop();
        bottom_set_mode(BOTTOM_MODE_DEFAULT);
        lastRomListIndex = -1;
        currentState = nav_pop();
    } else if (result == ROMS_SELECTED) {
        sound_play_click();
        int romId = roms_get_id_at(roms_get_selected_index());
        if (romId >= 0) {
            open_rom_detail(romId, platforms[selectedPlatformIndex].slug);
        }
    } else if (result == ROMS_LOAD_MORE) {
        show_loading("Loading more ROMs...");
        int offset = roms_get_count();
        int newCount, newTotal;
        log_info("Loading more ROMs (offset %d)...", offset);
        Rom *moreRoms = api_get_roms(platforms[selectedPlatformIndex].id, offset, ROM_PAGE_SIZE, &newCount, &newTotal);
        if (moreRoms) {
            log_info("Loaded %d more ROMs", newCount);
            roms_append_data(moreRoms, newCount);
        }
    }
}

static void handle_state_rom_detail(u32 kDown) {
    RomDetailResult result = romdetail_update(kDown);
    if (result == ROMDETAIL_BACK) {
        sound_play_pop();
        AppState returnState = nav_pop();
        if (returnState == STATE_QUEUE) {
            queue_screen_init();
            bottom_set_mode(BOTTOM_MODE_QUEUE);
            bottom_set_queue_count(queue_count());
            currentState = returnState;
        } else {
            sync_bottom_after_action(returnState);
        }
    }
}

static void handle_state_select_folder(u32 kDown, BottomAction bottomAction) {
    browser_update(kDown);
    bottom_set_folder_name(browser_get_current_name());

    if (bottomAction == BOTTOM_ACTION_CREATE_FOLDER) {
        char newName[256];
        if (browser_prompt_folder_name(newName, sizeof(newName))) {
            sound_play_click();
            show_loading("Selecting folder...");
            if (browser_create_folder(newName)) {
                browser_select_current();
                bottomAction = BOTTOM_ACTION_SELECT_FOLDER;
            }
        }
    }

    if (bottomAction == BOTTOM_ACTION_SELECT_FOLDER) {
        sound_play_click();
        if (browser_select_current()) {
            const char *folderName = browser_get_selected_folder_name();
            config_set_platform_folder(&config, currentPlatformSlug, folderName);
            browser_exit();

            // Restore the originating state so get_focused_rom works
            AppState returnState = folderPickerReturnState;
            currentState = returnState;

            if (queueAddPending) {
                queueAddPending = false;
                const char *slug;
                Rom rom;
                if (get_focused_rom(&rom, &slug)) {
                    if (queue_add(rom.id, rom.platformId, rom.name, rom.fsName, slug)) {
                        log_info("Added '%s' to download queue", rom.name);
                    }
                }
                sync_bottom_after_action(returnState);
            } else {
                const char *slug;
                Rom rom;
                if (get_focused_rom(&rom, &slug)) {
                    download_focused_rom(&rom, slug, folderName);
                }
                sync_bottom_after_action(returnState);
            }
        }
    }

    if (browser_was_cancelled()) {
        sound_play_pop();
        browser_exit();
        queueAddPending = false;
        sync_bottom_after_action(folderPickerReturnState);
    }
}

static void handle_state_queue(u32 kDown) {
    QueueResult qResult = queue_screen_update(kDown);
    if (qResult == QUEUE_BACK) {
        sound_play_pop();
        bottom_set_mode(BOTTOM_MODE_DEFAULT);
        currentState = nav_pop();
    } else if (qResult == QUEUE_SELECTED) {
        sound_play_click();
        QueueEntry *entry = queue_get(queue_screen_get_selected_index());
        if (entry) {
            open_rom_detail(entry->romId, entry->platformSlug);
        }
    }
}

static void handle_state_search_form(u32 kDown) {
    SearchFormResult sfResult = search_form_update(kDown);
    if (sfResult == SEARCH_FORM_BACK) {
        sound_play_pop();
        bottom_set_mode(BOTTOM_MODE_DEFAULT);
        currentState = nav_pop();
    } else if (sfResult == SEARCH_FORM_EXECUTE) {
        execute_search();
    }
}

static void handle_state_search_results(u32 kDown) {
    SearchResultsResult srResult = search_results_update(kDown);

    if (srResult == SEARCH_RESULTS_BACK) {
        sound_play_pop();
        bottom_set_mode(BOTTOM_MODE_SEARCH_FORM);
        currentState = nav_pop();
        return;
    }

    int curSearchIdx = search_get_selected_index();
    if (curSearchIdx != lastSearchListIndex) {
        const Rom *curSearchRom = search_get_result_at(curSearchIdx);
        if (curSearchRom) {
            const char *slug = search_get_platform_slug(curSearchRom->platformId);
            bottom_set_rom_exists(check_file_exists(slug, curSearchRom->fsName));
            bottom_set_rom_queued(queue_contains(curSearchRom->id));
        }
        lastSearchListIndex = curSearchIdx;
    }

    if (srResult == SEARCH_RESULTS_SELECTED) {
        sound_play_click();
        int romId = search_get_result_id_at(curSearchIdx);
        if (romId >= 0) {
            const Rom *selRom = search_get_result_at(curSearchIdx);
            open_rom_detail(romId, selRom ? search_get_platform_slug(selRom->platformId) : "");
        }
    } else if (srResult == SEARCH_RESULTS_LOAD_MORE) {
        show_loading("Loading more results...");
        int offset = search_get_result_count();
        int idCount;
        const int *ids = search_get_platform_ids(&idCount);
        int moreCount, moreTotal;
        Rom *moreResults =
            api_search_roms(search_get_term(), ids, idCount, offset, ROM_PAGE_SIZE, &moreCount, &moreTotal);
        if (moreResults) {
            log_info("Loaded %d more results", moreCount);
            search_append_results(moreResults, moreCount);
        }
    }
}

static void handle_state_about(u32 kDown) {
    AboutResult result = about_update(kDown);
    if (result == ABOUT_BACK) {
        sound_play_pop();
        bottom_set_mode(BOTTOM_MODE_DEFAULT);
        currentState = nav_pop();
    }
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

static void draw_top_screen(void) {
    switch (currentState) {
    case STATE_LOADING:
        ui_draw_text(160, 120, "Loading...", UI_COLOR_TEXT);
        break;
    case STATE_SETTINGS:
        settings_draw();
        break;
    case STATE_PLATFORMS:
        platforms_draw();
        break;
    case STATE_ROMS:
        roms_draw();
        break;
    case STATE_ROM_DETAIL:
        romdetail_draw();
        break;
    case STATE_SELECT_ROM_FOLDER:
        browser_draw();
        break;
    case STATE_QUEUE:
        queue_screen_draw();
        break;
    case STATE_SEARCH_FORM:
        ui_draw_wrapped_text(
            UI_PADDING, SCREEN_TOP_HEIGHT / 2 - UI_LINE_HEIGHT, SCREEN_TOP_WIDTH - UI_PADDING * 2,
            "Enter a search term and tap the Search button to find ROMs across the selected platforms.",
            UI_COLOR_TEXT_DIM, 0, 0);
        break;
    case STATE_SEARCH_RESULTS:
        search_results_draw();
        break;
    case STATE_ABOUT:
        about_draw();
        break;
    }
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    romfsInit();
    httpcInit(0);

    ui_init();
    sound_init();
    log_init();
    config_init(&config);
    mkdir(CONFIG_DIR, 0755);
    api_init();

    if (!config_load(&config)) {
        needsConfigSetup = true;
    } else {
        api_set_base_url(config.serverUrl);
        api_set_auth(config.username, config.password);
    }

    settings_init(&config);
    platforms_init();
    roms_init();
    romdetail_init();
    bottom_init();
    queue_init();
    queue_screen_init();
    debuglog_init();

    log_subscribe(debuglog_subscriber);
    log_info("Rommlet - RomM Client");

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        BottomAction bottomAction = bottom_update();

        if (kDown & KEY_START) break;

        handle_bottom_action(bottomAction);

        switch (currentState) {
        case STATE_LOADING:
            handle_state_loading();
            break;
        case STATE_SETTINGS:
            handle_state_settings(kDown);
            break;
        case STATE_PLATFORMS:
            handle_state_platforms(kDown);
            break;
        case STATE_ROMS:
            handle_state_roms(kDown);
            break;
        case STATE_ROM_DETAIL:
            handle_state_rom_detail(kDown);
            break;
        case STATE_SELECT_ROM_FOLDER:
            handle_state_select_folder(kDown, bottomAction);
            break;
        case STATE_QUEUE:
            handle_state_queue(kDown);
            break;
        case STATE_SEARCH_FORM:
            handle_state_search_form(kDown);
            break;
        case STATE_SEARCH_RESULTS:
            handle_state_search_results(kDown);
            break;
        case STATE_ABOUT:
            handle_state_about(kDown);
            break;
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topScreen, UI_COLOR_BG);
        C2D_SceneBegin(topScreen);
        draw_top_screen();
        bottom_draw();
        C3D_FrameEnd(0);
    }

    if (platforms) api_free_platforms(platforms, platformCount);
    roms_clear();
    if (romDetail) api_free_rom_detail(romDetail);

    bottom_exit();
    sound_exit();
    ui_exit();
    api_exit();

    httpcExit();
    romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();

    return 0;
}
