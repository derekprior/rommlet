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
#include <3ds.h>
#include <citro2d.h>
#include "config.h"
#include "api.h"
#include "log.h"
#include "ui.h"
#include "browser.h"
#include "screens/settings.h"
#include "screens/platforms.h"
#include "screens/roms.h"
#include "screens/romdetail.h"
#include "screens/bottom.h"
#include "queue.h"
#include "screens/queuescreen.h"
#include "screens/search.h"

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
    STATE_SEARCH_RESULTS
} AppState;

static AppState currentState = STATE_LOADING;
static AppState previousState = STATE_PLATFORMS;  // For returning from settings
static bool needsConfigSetup = false;
static bool cameFromQueue = false;  // Track if ROM detail was opened from queue
static bool cameFromSearch = false;  // Track if ROM detail was opened from search
static bool queueAddPending = false;  // Track if folder selection is for queue add
static AppState folderBrowserReturnState = STATE_ROM_DETAIL;  // State to return to after folder browser
static bool queueConfirmShown = false;  // Track if clear-queue confirmation is showing

// Shared state
static Config config;
static Platform *platforms = NULL;
static int platformCount = 0;
static int selectedPlatformIndex = 0;
static RomDetail *romDetail = NULL;
static int selectedRomIndex = 0;
static int lastRomListIndex = -1;  // Track selection changes in ROM list
static char currentPlatformSlug[CONFIG_MAX_SLUG_LEN];  // For folder mapping

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
static const char *downloadName = NULL;
static const char *downloadQueueText = NULL;

// Download progress callback - renders progress bar each chunk
// Returns true to continue, false to cancel
static bool download_progress(u32 downloaded, u32 total) {
    float progress = total > 0 ? (float)downloaded / total : -1.0f;
    
    char sizeText[64];
    if (total > 0) {
        snprintf(sizeText, sizeof(sizeText), "%.1f / %.1f MB",
                 downloaded / (1024.0f * 1024.0f), total / (1024.0f * 1024.0f));
    } else {
        snprintf(sizeText, sizeof(sizeText), "%.1f MB downloaded",
                 downloaded / (1024.0f * 1024.0f));
    }
    
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(topScreen, UI_COLOR_BG);
    C2D_SceneBegin(topScreen);
    ui_draw_download_progress(progress, sizeText, downloadName, downloadQueueText);
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

// Check if the current ROM file already exists on disk
static bool check_rom_exists(void) {
    if (!romDetail) return false;
    const char *folderName = config_get_platform_folder(currentPlatformSlug);
    if (!folderName || !folderName[0]) return false;
    char path[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 256 + 3];
    snprintf(path, sizeof(path), "%s/%s/%s", config.romFolder, folderName, romDetail->fileName);
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

// Check if a ROM file exists on disk by filename
static bool check_file_exists(const char *platformSlug, const char *fileName) {
    const char *folderName = config_get_platform_folder(platformSlug);
    if (!folderName || !folderName[0]) return false;
    char path[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 256 + 3];
    snprintf(path, sizeof(path), "%s/%s/%s", config.romFolder, folderName, fileName);
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

// Check if a platform folder is configured and valid on disk
static bool check_platform_folder_valid(const char *platformSlug) {
    const char *folderName = config_get_platform_folder(platformSlug);
    if (!folderName || !folderName[0]) {
        log_info("No folder configured for platform '%s'", platformSlug);
        return false;
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

// Update bottom screen state for the selected ROM in the list
static void sync_roms_bottom(int index) {
    const Rom *rom = roms_get_at(index);
    if (rom) {
        bottom_set_rom_exists(check_file_exists(currentPlatformSlug, rom->fsName));
        bottom_set_rom_queued(queue_contains(rom->id));
    }
    lastRomListIndex = index;
}

// Add the current ROM to the queue (assumes folder is valid)
static void add_current_rom_to_queue(void) {
    if (!romDetail) return;
    if (queue_add(romDetail->id, romDetail->platformId, romDetail->name,
                  romDetail->fileName, currentPlatformSlug, romDetail->platformName)) {
        log_info("Added '%s' to download queue", romDetail->name);
    }
    bottom_set_rom_queued(queue_contains(romDetail->id));
    bottom_set_queue_count(queue_count());
}

// Download a single queue entry. Returns true on success.
static bool download_queue_entry(QueueEntry *entry) {
    const char *folderName = config_get_platform_folder(entry->platformSlug);
    if (!folderName || !folderName[0]) {
        log_error("No folder for platform '%s', skipping", entry->platformSlug);
        return false;
    }
    char destPath[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 256 + 3];
    snprintf(destPath, sizeof(destPath), "%s/%s/%s", config.romFolder, folderName, entry->fileName);
    log_info("Downloading '%s' to: %s", entry->name, destPath);
    return api_download_rom(entry->romId, entry->fileName, destPath, download_progress);
}

int main(int argc, char *argv[]) {
    // Initialize services
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    
    // Initialize render targets
    topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    
    // Initialize subsystems
    romfsInit();
    httpcInit(0);
    
    // Initialize modules
    ui_init();
    log_init();
    config_init(&config);
    api_init();
    
    // Load configuration
    if (!config_load(&config)) {
        needsConfigSetup = true;
    } else {
        // Set up API with loaded config
        api_set_base_url(config.serverUrl);
        api_set_auth(config.username, config.password);
    }
    
    // Initialize screens
    settings_init(&config);
    platforms_init();
    roms_init();
    romdetail_init();
    bottom_init();
    queue_init();
    queue_screen_init();
    
    // Register bottom screen as log subscriber
    log_subscribe(bottom_log_subscriber);
    log_info("Rommlet - RomM Client");
    
    // Main loop
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        
        // Let bottom screen handle touch and ZL/ZR first
        BottomAction bottomAction = bottom_update();
        
        // Global exit
        if (kDown & KEY_START) {
            break;
        }
        
        // Handle bottom screen actions
        if (bottomAction == BOTTOM_ACTION_SAVE_SETTINGS && currentState == STATE_SETTINGS) {
            config_save(&config);
            api_set_auth(config.username, config.password);
            api_set_base_url(config.serverUrl);
            bottom_set_mode(BOTTOM_MODE_DEFAULT);
            currentState = STATE_PLATFORMS;
            fetch_platforms();
        }
        
        // Handle cancel from bottom screen button
        if (bottomAction == BOTTOM_ACTION_CANCEL_SETTINGS && currentState == STATE_SETTINGS) {
            bottom_set_mode(BOTTOM_MODE_DEFAULT);
            currentState = previousState;
        }
        
        // Handle opening settings from toolbar
        if (bottomAction == BOTTOM_ACTION_OPEN_SETTINGS && currentState != STATE_SETTINGS) {
            previousState = currentState;
            bottom_set_settings_mode(config_is_valid(&config));
            currentState = STATE_SETTINGS;
        }
        
        // Handle home button - return to platform list
        if (bottomAction == BOTTOM_ACTION_GO_HOME && currentState != STATE_PLATFORMS) {
            bottom_set_mode(BOTTOM_MODE_DEFAULT);
            lastRomListIndex = -1;
            cameFromQueue = false;
            cameFromSearch = false;
            queueAddPending = false;
            currentState = STATE_PLATFORMS;
        }
        
        // Handle download ROM from detail screen
        if (bottomAction == BOTTOM_ACTION_DOWNLOAD_ROM && currentState == STATE_ROM_DETAIL && romDetail) {
            queueAddPending = false;
            if (check_platform_folder_valid(currentPlatformSlug)) {
                const char *folderName = config_get_platform_folder(currentPlatformSlug);
                log_info("Using folder '%s' for platform '%s'", folderName, currentPlatformSlug);
                char destPath[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 256 + 3];
                snprintf(destPath, sizeof(destPath), "%s/%s/%s", 
                        config.romFolder, folderName, romDetail->fileName);
                bottom_set_mode(BOTTOM_MODE_DOWNLOADING);
                downloadName = romDetail->name;
                downloadQueueText = NULL;
                log_info("Downloading to: %s", destPath);
                if (api_download_rom(romDetail->id, romDetail->fileName, destPath, download_progress)) {
                    log_info("Download complete!");
                } else {
                    log_error("Download failed!");
                }
                bottom_set_mode(BOTTOM_MODE_ROM_DETAIL);
                bottom_set_rom_exists(check_rom_exists());
            } else {
                // No mapping or folder doesn't exist - show folder browser
                browser_init_rooted(config.romFolder, currentPlatformSlug);
                bottom_set_mode(BOTTOM_MODE_FOLDER_BROWSER);
                folderBrowserReturnState = STATE_ROM_DETAIL;
                currentState = STATE_SELECT_ROM_FOLDER;
            }
        }
        
        // Handle download ROM from ROM list
        if (bottomAction == BOTTOM_ACTION_DOWNLOAD_ROM && currentState == STATE_ROMS) {
            const Rom *rom = roms_get_at(roms_get_selected_index());
            if (rom) {
                queueAddPending = false;
                if (check_platform_folder_valid(currentPlatformSlug)) {
                    const char *folderName = config_get_platform_folder(currentPlatformSlug);
                    char destPath[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 256 + 3];
                    snprintf(destPath, sizeof(destPath), "%s/%s/%s",
                            config.romFolder, folderName, rom->fsName);
                    bottom_set_mode(BOTTOM_MODE_DOWNLOADING);
                    downloadName = rom->name;
                    downloadQueueText = NULL;
                    log_info("Downloading to: %s", destPath);
                    if (api_download_rom(rom->id, rom->fsName, destPath, download_progress)) {
                        log_info("Download complete!");
                    } else {
                        log_error("Download failed!");
                    }
                    bottom_set_mode(BOTTOM_MODE_ROMS);
                    sync_roms_bottom(roms_get_selected_index());
                } else {
                    browser_init_rooted(config.romFolder, currentPlatformSlug);
                    bottom_set_mode(BOTTOM_MODE_FOLDER_BROWSER);
                    folderBrowserReturnState = STATE_ROMS;
                    currentState = STATE_SELECT_ROM_FOLDER;
                }
            }
        }
        
        // Handle add/remove queue from ROM detail screen
        if (bottomAction == BOTTOM_ACTION_QUEUE_ROM && currentState == STATE_ROM_DETAIL && romDetail) {
            if (queue_contains(romDetail->id)) {
                // Remove from queue
                queue_remove(romDetail->id);
                log_info("Removed '%s' from download queue", romDetail->name);
                bottom_set_rom_queued(false);
                bottom_set_queue_count(queue_count());
            } else {
                // Add to queue - but first validate folder
                if (check_platform_folder_valid(currentPlatformSlug)) {
                    add_current_rom_to_queue();
                } else {
                    queueAddPending = true;
                    browser_init_rooted(config.romFolder, currentPlatformSlug);
                    bottom_set_mode(BOTTOM_MODE_FOLDER_BROWSER);
                    folderBrowserReturnState = STATE_ROM_DETAIL;
                    currentState = STATE_SELECT_ROM_FOLDER;
                }
            }
        }
        
        // Handle add/remove queue from ROM list
        if (bottomAction == BOTTOM_ACTION_QUEUE_ROM && currentState == STATE_ROMS) {
            const Rom *rom = roms_get_at(roms_get_selected_index());
            if (rom) {
                if (queue_contains(rom->id)) {
                    queue_remove(rom->id);
                    log_info("Removed '%s' from download queue", rom->name);
                    bottom_set_rom_queued(false);
                    bottom_set_queue_count(queue_count());
                } else {
                    if (check_platform_folder_valid(currentPlatformSlug)) {
                        if (queue_add(rom->id, rom->platformId, rom->name, rom->fsName,
                                      currentPlatformSlug, platforms[selectedPlatformIndex].displayName)) {
                            log_info("Added '%s' to download queue", rom->name);
                        }
                        bottom_set_rom_queued(queue_contains(rom->id));
                        bottom_set_queue_count(queue_count());
                    } else {
                        queueAddPending = true;
                        browser_init_rooted(config.romFolder, currentPlatformSlug);
                        bottom_set_mode(BOTTOM_MODE_FOLDER_BROWSER);
                        folderBrowserReturnState = STATE_ROMS;
                        currentState = STATE_SELECT_ROM_FOLDER;
                    }
                }
            }
        }
        
        // Handle opening queue from toolbar
        if (bottomAction == BOTTOM_ACTION_OPEN_QUEUE && currentState != STATE_QUEUE) {
            previousState = currentState;
            queue_clear_failed();
            queue_screen_init();
            bottom_set_mode(BOTTOM_MODE_QUEUE);
            bottom_set_queue_count(queue_count());
            currentState = STATE_QUEUE;
        }
        
        // Handle opening search from toolbar
        if (bottomAction == BOTTOM_ACTION_OPEN_SEARCH) {
            if (currentState != STATE_SEARCH_FORM) {
                previousState = currentState == STATE_SEARCH_RESULTS ? previousState : currentState;
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
        }
        
        // Handle search field tap (reopen keyboard)
        if (bottomAction == BOTTOM_ACTION_SEARCH_FIELD && currentState == STATE_SEARCH_FORM) {
            search_open_keyboard();
        }
        
        // Handle search execute from touch button
        if (bottomAction == BOTTOM_ACTION_SEARCH_EXECUTE && currentState == STATE_SEARCH_FORM) {
            const char *term = search_get_term();
            if (term && term[0]) {
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
                bottom_set_mode(BOTTOM_MODE_SEARCH_RESULTS);
                bottom_set_queue_count(queue_count());
                // Sync bottom for first result
                const Rom *firstRom = search_get_result_at(0);
                if (firstRom) {
                    const char *slug = search_get_platform_slug(firstRom->platformId);
                    bottom_set_rom_exists(check_file_exists(slug, firstRom->fsName));
                    bottom_set_rom_queued(queue_contains(firstRom->id));
                }
                currentState = STATE_SEARCH_RESULTS;
            }
        }
        
        // Handle download ROM from search results
        if (bottomAction == BOTTOM_ACTION_DOWNLOAD_ROM && currentState == STATE_SEARCH_RESULTS) {
            const Rom *rom = search_get_result_at(search_get_selected_index());
            if (rom) {
                const char *slug = search_get_platform_slug(rom->platformId);
                queueAddPending = false;
                if (check_platform_folder_valid(slug)) {
                    const char *folderName = config_get_platform_folder(slug);
                    char destPath[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 256 + 3];
                    snprintf(destPath, sizeof(destPath), "%s/%s/%s",
                            config.romFolder, folderName, rom->fsName);
                    bottom_set_mode(BOTTOM_MODE_DOWNLOADING);
                    downloadName = rom->name;
                    downloadQueueText = NULL;
                    log_info("Downloading to: %s", destPath);
                    if (api_download_rom(rom->id, rom->fsName, destPath, download_progress)) {
                        log_info("Download complete!");
                    } else {
                        log_error("Download failed!");
                    }
                    bottom_set_mode(BOTTOM_MODE_SEARCH_RESULTS);
                    bottom_set_rom_exists(check_file_exists(slug, rom->fsName));
                    bottom_set_rom_queued(queue_contains(rom->id));
                } else {
                    snprintf(currentPlatformSlug, sizeof(currentPlatformSlug), "%s", slug);
                    browser_init_rooted(config.romFolder, slug);
                    bottom_set_mode(BOTTOM_MODE_FOLDER_BROWSER);
                    folderBrowserReturnState = STATE_SEARCH_RESULTS;
                    currentState = STATE_SELECT_ROM_FOLDER;
                }
            }
        }
        
        // Handle add/remove queue from search results
        if (bottomAction == BOTTOM_ACTION_QUEUE_ROM && currentState == STATE_SEARCH_RESULTS) {
            const Rom *rom = search_get_result_at(search_get_selected_index());
            if (rom) {
                const char *slug = search_get_platform_slug(rom->platformId);
                const char *platName = search_get_platform_name(rom->platformId);
                if (queue_contains(rom->id)) {
                    queue_remove(rom->id);
                    log_info("Removed '%s' from download queue", rom->name);
                    bottom_set_rom_queued(false);
                    bottom_set_queue_count(queue_count());
                } else {
                    if (check_platform_folder_valid(slug)) {
                        if (queue_add(rom->id, rom->platformId, rom->name, rom->fsName,
                                      slug, platName)) {
                            log_info("Added '%s' to download queue", rom->name);
                        }
                        bottom_set_rom_queued(queue_contains(rom->id));
                        bottom_set_queue_count(queue_count());
                    } else {
                        snprintf(currentPlatformSlug, sizeof(currentPlatformSlug), "%s", slug);
                        queueAddPending = true;
                        browser_init_rooted(config.romFolder, slug);
                        bottom_set_mode(BOTTOM_MODE_FOLDER_BROWSER);
                        folderBrowserReturnState = STATE_SEARCH_RESULTS;
                        currentState = STATE_SELECT_ROM_FOLDER;
                    }
                }
            }
        }
        
        // Handle start downloads from queue screen
        if (bottomAction == BOTTOM_ACTION_START_DOWNLOADS && currentState == STATE_QUEUE) {
            int count = queue_count();
            if (count > 0) {
                bottom_set_mode(BOTTOM_MODE_DOWNLOADING);
                int completed = 0;
                int i = 0;
                while (i < queue_count()) {
                    QueueEntry *entry = queue_get(i);
                    if (!entry) { i++; continue; }
                    
                    char queueText[64];
                    snprintf(queueText, sizeof(queueText), "ROM %d of %d in your queue", completed + 1, count);
                    downloadName = entry->name;
                    downloadQueueText = queueText;
                    
                    if (download_queue_entry(entry)) {
                        log_info("Queue download complete: %s", entry->name);
                        queue_remove(entry->romId);
                        completed++;
                        // Don't increment i — array shifted
                    } else {
                        log_error("Queue download failed: %s", entry->name);
                        queue_set_failed(i, true);
                        i++;
                    }
                }
                // Return to queue view
                downloadName = NULL;
                downloadQueueText = NULL;
                bottom_set_mode(BOTTOM_MODE_QUEUE);
                bottom_set_queue_count(queue_count());
                queue_screen_init();
            }
        }
        
        // Handle clear queue (show confirmation)
        if (bottomAction == BOTTOM_ACTION_CLEAR_QUEUE && currentState == STATE_QUEUE) {
            if (queueConfirmShown) {
                // Confirmed - actually clear
                queueConfirmShown = false;
                queue_clear();
                log_info("Download queue cleared");
                bottom_set_mode(BOTTOM_MODE_QUEUE);
                bottom_set_queue_count(0);
                queue_screen_init();
            } else {
                // Show confirmation
                queueConfirmShown = true;
                bottom_set_mode(BOTTOM_MODE_QUEUE_CONFIRM);
            }
        }
        
        // Handle cancel clear queue confirmation
        if (bottomAction == BOTTOM_ACTION_CANCEL_CLEAR && currentState == STATE_QUEUE) {
            queueConfirmShown = false;
            bottom_set_mode(BOTTOM_MODE_QUEUE);
        }
        
        // Handle state-specific input and updates
        switch (currentState) {
            case STATE_LOADING:
                // Transition to appropriate screen
                if (needsConfigSetup) {
                    currentState = STATE_SETTINGS;
                    bottom_set_settings_mode(false);  // No cancel on first run
                } else {
                    currentState = STATE_PLATFORMS;
                    bottom_set_mode(BOTTOM_MODE_DEFAULT);
                    fetch_platforms();
                }
                break;
                
            case STATE_SETTINGS: {
                SettingsResult result = settings_update(kDown);
                if (result == SETTINGS_SAVED) {
                    config_save(&config);
                    api_set_auth(config.username, config.password);
                    api_set_base_url(config.serverUrl);
                    bottom_set_mode(BOTTOM_MODE_DEFAULT);
                    currentState = STATE_PLATFORMS;
                    fetch_platforms();
                } else if (result == SETTINGS_CANCELLED) {
                    if (config_is_valid(&config)) {
                        bottom_set_mode(BOTTOM_MODE_DEFAULT);
                        currentState = previousState;
                    } else {
                        log_warn("Configuration not valid. Please complete all fields.");
                    }
                }
                break;
            }
                
            case STATE_PLATFORMS: {
                PlatformsResult result = platforms_update(kDown, &selectedPlatformIndex);
                if (result == PLATFORMS_SELECTED && platforms && selectedPlatformIndex < platformCount) {
                    // Fetch ROMs for selected platform
                    show_loading("Fetching ROMs...");
                    log_info("Fetching ROMs for %s...", platforms[selectedPlatformIndex].displayName);
                    roms_clear();
                    int romCount, romTotal;
                    Rom *roms = api_get_roms(platforms[selectedPlatformIndex].id, 0, ROM_PAGE_SIZE, &romCount, &romTotal);
                    if (roms) {
                        log_info("Found %d/%d ROMs", romCount, romTotal);
                        roms_set_data(roms, romCount, romTotal, platforms[selectedPlatformIndex].displayName);
                        snprintf(currentPlatformSlug, sizeof(currentPlatformSlug), "%s",
                                platforms[selectedPlatformIndex].slug);
                        lastRomListIndex = -1;
                        bottom_set_mode(BOTTOM_MODE_ROMS);
                        bottom_set_queue_count(queue_count());
                        sync_roms_bottom(0);
                        currentState = STATE_ROMS;
                    } else {
                        log_error("Failed to fetch ROMs");
                    }
                } else if (result == PLATFORMS_REFRESH) {
                    fetch_platforms();
                }
                break;
            }
                
            case STATE_ROMS: {
                RomsResult result = roms_update(kDown, &selectedRomIndex);
                
                // Sync bottom screen when selection changes
                int curIdx = roms_get_selected_index();
                if (curIdx != lastRomListIndex && curIdx < roms_get_count()) {
                    sync_roms_bottom(curIdx);
                }
                
                if (result == ROMS_BACK) {
                    bottom_set_mode(BOTTOM_MODE_DEFAULT);
                    lastRomListIndex = -1;
                    currentState = STATE_PLATFORMS;
                } else if (result == ROMS_SELECTED) {
                    // Fetch ROM details
                    int romId = roms_get_id_at(selectedRomIndex);
                    if (romId >= 0) {
                        show_loading("Loading ROM details...");
                        log_info("Fetching ROM details for ID %d...", romId);
                        if (romDetail) {
                            api_free_rom_detail(romDetail);
                            romDetail = NULL;
                        }
                        romDetail = api_get_rom_detail(romId);
                        if (romDetail) {
                            romdetail_set_data(romDetail);
                            // Track platform slug for folder mapping
                            snprintf(currentPlatformSlug, sizeof(currentPlatformSlug), "%s", 
                                    platforms[selectedPlatformIndex].slug);
                            cameFromQueue = false;
                            bottom_set_mode(BOTTOM_MODE_ROM_DETAIL);
                            bottom_set_rom_exists(check_rom_exists());
                            bottom_set_rom_queued(queue_contains(romDetail->id));
                            bottom_set_queue_count(queue_count());
                            currentState = STATE_ROM_DETAIL;
                        } else {
                            log_error("Failed to fetch ROM details");
                        }
                    }
                } else if (result == ROMS_LOAD_MORE) {
                    // Fetch next page synchronously
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
                break;
            }
            
            case STATE_ROM_DETAIL: {
                RomDetailResult result = romdetail_update(kDown);
                if (result == ROMDETAIL_BACK) {
                    if (cameFromQueue) {
                        cameFromQueue = false;
                        queue_screen_init();
                        bottom_set_mode(BOTTOM_MODE_QUEUE);
                        bottom_set_queue_count(queue_count());
                        currentState = STATE_QUEUE;
                    } else if (cameFromSearch) {
                        cameFromSearch = false;
                        bottom_set_mode(BOTTOM_MODE_SEARCH_RESULTS);
                        bottom_set_queue_count(queue_count());
                        const Rom *sr = search_get_result_at(search_get_selected_index());
                        if (sr) {
                            const char *slug = search_get_platform_slug(sr->platformId);
                            bottom_set_rom_exists(check_file_exists(slug, sr->fsName));
                            bottom_set_rom_queued(queue_contains(sr->id));
                        }
                        currentState = STATE_SEARCH_RESULTS;
                    } else {
                        bottom_set_mode(BOTTOM_MODE_ROMS);
                        sync_roms_bottom(roms_get_selected_index());
                        currentState = STATE_ROMS;
                    }
                }
                break;
            }
            
            case STATE_SELECT_ROM_FOLDER: {
                browser_update(kDown);
                
                // Update folder name for bottom screen button
                bottom_set_folder_name(browser_get_current_name());
                
                // Handle create folder from bottom button
                if (bottomAction == BOTTOM_ACTION_CREATE_FOLDER) {
                    browser_create_folder();
                    bottom_set_folder_name(browser_get_current_name());
                }
                
                // Handle select folder from bottom button
                if (bottomAction == BOTTOM_ACTION_SELECT_FOLDER) {
                    if (browser_select_current()) {
                        const char *folderName = browser_get_selected_folder_name();
                        config_set_platform_folder(&config, currentPlatformSlug, folderName);
                        browser_exit();
                        
                        if (queueAddPending) {
                            queueAddPending = false;
                            if (folderBrowserReturnState == STATE_ROMS) {
                                const Rom *rom = roms_get_at(roms_get_selected_index());
                                if (rom) {
                                    if (queue_add(rom->id, rom->platformId, rom->name, rom->fsName,
                                                  currentPlatformSlug, platforms[selectedPlatformIndex].displayName)) {
                                        log_info("Added '%s' to download queue", rom->name);
                                    }
                                }
                                bottom_set_mode(BOTTOM_MODE_ROMS);
                                bottom_set_queue_count(queue_count());
                                sync_roms_bottom(roms_get_selected_index());
                            } else if (folderBrowserReturnState == STATE_SEARCH_RESULTS) {
                                const Rom *rom = search_get_result_at(search_get_selected_index());
                                if (rom) {
                                    const char *platName = search_get_platform_name(rom->platformId);
                                    if (queue_add(rom->id, rom->platformId, rom->name, rom->fsName,
                                                  currentPlatformSlug, platName)) {
                                        log_info("Added '%s' to download queue", rom->name);
                                    }
                                }
                                bottom_set_mode(BOTTOM_MODE_SEARCH_RESULTS);
                                bottom_set_queue_count(queue_count());
                                if (rom) {
                                    bottom_set_rom_exists(check_file_exists(currentPlatformSlug, rom->fsName));
                                    bottom_set_rom_queued(queue_contains(rom->id));
                                }
                            } else {
                                add_current_rom_to_queue();
                                bottom_set_mode(BOTTOM_MODE_ROM_DETAIL);
                                bottom_set_rom_exists(check_rom_exists());
                            }
                            currentState = folderBrowserReturnState;
                        } else {
                            // Was downloading - now download
                            if (folderBrowserReturnState == STATE_ROMS) {
                                const Rom *rom = roms_get_at(roms_get_selected_index());
                                if (rom) {
                                    char destPath[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 256 + 3];
                                    snprintf(destPath, sizeof(destPath), "%s/%s/%s",
                                            config.romFolder, folderName, rom->fsName);
                                    bottom_set_mode(BOTTOM_MODE_DOWNLOADING);
                                    downloadName = rom->name;
                                    downloadQueueText = NULL;
                                    log_info("Downloading to: %s", destPath);
                                    if (api_download_rom(rom->id, rom->fsName, destPath, download_progress)) {
                                        log_info("Download complete!");
                                    } else {
                                        log_error("Download failed!");
                                    }
                                }
                                bottom_set_mode(BOTTOM_MODE_ROMS);
                                sync_roms_bottom(roms_get_selected_index());
                            } else if (folderBrowserReturnState == STATE_SEARCH_RESULTS) {
                                const Rom *rom = search_get_result_at(search_get_selected_index());
                                if (rom) {
                                    char destPath[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 256 + 3];
                                    snprintf(destPath, sizeof(destPath), "%s/%s/%s",
                                            config.romFolder, folderName, rom->fsName);
                                    bottom_set_mode(BOTTOM_MODE_DOWNLOADING);
                                    downloadName = rom->name;
                                    downloadQueueText = NULL;
                                    log_info("Downloading to: %s", destPath);
                                    if (api_download_rom(rom->id, rom->fsName, destPath, download_progress)) {
                                        log_info("Download complete!");
                                    } else {
                                        log_error("Download failed!");
                                    }
                                }
                                bottom_set_mode(BOTTOM_MODE_SEARCH_RESULTS);
                                if (rom) {
                                    bottom_set_rom_exists(check_file_exists(currentPlatformSlug, rom->fsName));
                                    bottom_set_rom_queued(queue_contains(rom->id));
                                }
                            } else {
                                if (romDetail) {
                                    char destPath[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 256 + 3];
                                    snprintf(destPath, sizeof(destPath), "%s/%s/%s", 
                                            config.romFolder, folderName, romDetail->fileName);
                                    bottom_set_mode(BOTTOM_MODE_DOWNLOADING);
                                    downloadName = romDetail->name;
                                    downloadQueueText = NULL;
                                    log_info("Downloading to: %s", destPath);
                                    if (api_download_rom(romDetail->id, romDetail->fileName, destPath, download_progress)) {
                                        log_info("Download complete!");
                                    } else {
                                        log_error("Download failed!");
                                    }
                                }
                                bottom_set_mode(BOTTOM_MODE_ROM_DETAIL);
                                bottom_set_rom_exists(check_rom_exists());
                            }
                            currentState = folderBrowserReturnState;
                        }
                    }
                }
                
                if (browser_was_cancelled()) {
                    browser_exit();
                    queueAddPending = false;
                    if (folderBrowserReturnState == STATE_ROMS) {
                        bottom_set_mode(BOTTOM_MODE_ROMS);
                        sync_roms_bottom(roms_get_selected_index());
                    } else if (folderBrowserReturnState == STATE_SEARCH_RESULTS) {
                        bottom_set_mode(BOTTOM_MODE_SEARCH_RESULTS);
                        const Rom *rom = search_get_result_at(search_get_selected_index());
                        if (rom) {
                            const char *slug = search_get_platform_slug(rom->platformId);
                            bottom_set_rom_exists(check_file_exists(slug, rom->fsName));
                            bottom_set_rom_queued(queue_contains(rom->id));
                        }
                    } else {
                        bottom_set_mode(BOTTOM_MODE_ROM_DETAIL);
                        bottom_set_rom_exists(check_rom_exists());
                    }
                    currentState = folderBrowserReturnState;
                }
                break;
            }
            
            case STATE_QUEUE: {
                int selectedQueueIndex = 0;
                QueueResult qResult = queue_screen_update(kDown, &selectedQueueIndex);
                if (qResult == QUEUE_BACK) {
                    bottom_set_mode(BOTTOM_MODE_DEFAULT);
                    currentState = previousState;
                } else if (qResult == QUEUE_SELECTED) {
                    QueueEntry *entry = queue_get(selectedQueueIndex);
                    if (entry) {
                        show_loading("Loading ROM details...");
                        log_info("Fetching ROM details for queued ID %d...", entry->romId);
                        if (romDetail) {
                            api_free_rom_detail(romDetail);
                            romDetail = NULL;
                        }
                        romDetail = api_get_rom_detail(entry->romId);
                        if (romDetail) {
                            romdetail_set_data(romDetail);
                            snprintf(currentPlatformSlug, sizeof(currentPlatformSlug), "%s",
                                    entry->platformSlug);
                            cameFromQueue = true;
                            bottom_set_mode(BOTTOM_MODE_ROM_DETAIL);
                            bottom_set_rom_exists(check_rom_exists());
                            bottom_set_rom_queued(queue_contains(romDetail->id));
                            bottom_set_queue_count(queue_count());
                            currentState = STATE_ROM_DETAIL;
                        } else {
                            log_error("Failed to fetch ROM details");
                        }
                    }
                }
                break;
            }
            
            case STATE_SEARCH_FORM: {
                SearchFormResult sfResult = search_form_update(kDown);
                if (sfResult == SEARCH_FORM_BACK) {
                    bottom_set_mode(BOTTOM_MODE_DEFAULT);
                    currentState = previousState;
                } else if (sfResult == SEARCH_FORM_EXECUTE) {
                    const char *term = search_get_term();
                    if (term && term[0]) {
                        show_loading("Searching...");
                        int idCount;
                        const int *ids = search_get_platform_ids(&idCount);
                        int sCount, sTotal;
                        Rom *results = api_search_roms(term, ids, idCount, 0, ROM_PAGE_SIZE, &sCount, &sTotal);
                        if (results) {
                            log_info("Search found %d/%d results", sCount, sTotal);
                            search_set_results(results, sCount, sTotal);
                        } else {
                            log_info("Search returned no results");
                            search_set_results(NULL, 0, 0);
                        }
                        bottom_set_mode(BOTTOM_MODE_SEARCH_RESULTS);
                        bottom_set_queue_count(queue_count());
                        const Rom *firstRom = search_get_result_at(0);
                        if (firstRom) {
                            const char *slug = search_get_platform_slug(firstRom->platformId);
                            bottom_set_rom_exists(check_file_exists(slug, firstRom->fsName));
                            bottom_set_rom_queued(queue_contains(firstRom->id));
                        }
                        currentState = STATE_SEARCH_RESULTS;
                    }
                }
                break;
            }
            
            case STATE_SEARCH_RESULTS: {
                int searchSelectedIndex = 0;
                SearchResultsResult srResult = search_results_update(kDown, &searchSelectedIndex);
                
                // Sync bottom screen when selection changes
                int curSearchIdx = search_get_selected_index();
                const Rom *curSearchRom = search_get_result_at(curSearchIdx);
                if (curSearchRom) {
                    const char *slug = search_get_platform_slug(curSearchRom->platformId);
                    bottom_set_rom_exists(check_file_exists(slug, curSearchRom->fsName));
                    bottom_set_rom_queued(queue_contains(curSearchRom->id));
                }
                
                if (srResult == SEARCH_RESULTS_BACK) {
                    bottom_set_mode(BOTTOM_MODE_SEARCH_FORM);
                    currentState = STATE_SEARCH_FORM;
                } else if (srResult == SEARCH_RESULTS_SELECTED) {
                    int romId = search_get_result_id_at(searchSelectedIndex);
                    if (romId >= 0) {
                        show_loading("Loading ROM details...");
                        log_info("Fetching ROM details for ID %d...", romId);
                        if (romDetail) {
                            api_free_rom_detail(romDetail);
                            romDetail = NULL;
                        }
                        romDetail = api_get_rom_detail(romId);
                        if (romDetail) {
                            romdetail_set_data(romDetail);
                            snprintf(currentPlatformSlug, sizeof(currentPlatformSlug), "%s",
                                    search_get_platform_slug(romDetail->platformId));
                            cameFromSearch = true;
                            cameFromQueue = false;
                            bottom_set_mode(BOTTOM_MODE_ROM_DETAIL);
                            bottom_set_rom_exists(check_rom_exists());
                            bottom_set_rom_queued(queue_contains(romDetail->id));
                            bottom_set_queue_count(queue_count());
                            currentState = STATE_ROM_DETAIL;
                        } else {
                            log_error("Failed to fetch ROM details");
                        }
                    }
                } else if (srResult == SEARCH_RESULTS_LOAD_MORE) {
                    show_loading("Loading more results...");
                    int offset = search_get_result_count();
                    int idCount;
                    const int *ids = search_get_platform_ids(&idCount);
                    int moreCount, moreTotal;
                    Rom *moreResults = api_search_roms(search_get_term(), ids, idCount,
                                                       offset, ROM_PAGE_SIZE, &moreCount, &moreTotal);
                    if (moreResults) {
                        log_info("Loaded %d more results", moreCount);
                        search_append_results(moreResults, moreCount);
                    }
                }
                break;
            }
        }
        
        // Render
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topScreen, UI_COLOR_BG);
        C2D_SceneBegin(topScreen);
        
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
                ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT / 2 - UI_LINE_HEIGHT,
                             "Enter a search term and tap Search", UI_COLOR_TEXT_DIM);
                ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT / 2,
                             "to find ROMs across platforms.", UI_COLOR_TEXT_DIM);
                break;
            case STATE_SEARCH_RESULTS:
                search_results_draw();
                break;
        }
        
        // Draw bottom screen
        bottom_draw();
        
        C3D_FrameEnd(0);
    }
    
    // Cleanup
    if (platforms) api_free_platforms(platforms, platformCount);
    roms_clear();
    if (romDetail) api_free_rom_detail(romDetail);
    
    bottom_exit();
    ui_exit();
    api_exit();
    
    httpcExit();
    romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    
    return 0;
}
