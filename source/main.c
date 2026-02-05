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
#include "ui.h"
#include "browser.h"
#include "screens/settings.h"
#include "screens/platforms.h"
#include "screens/roms.h"
#include "screens/romdetail.h"
#include "screens/bottom.h"

// App states
typedef enum {
    STATE_LOADING,
    STATE_SETTINGS,
    STATE_PLATFORMS,
    STATE_ROMS,
    STATE_ROM_DETAIL,
    STATE_SELECT_ROM_FOLDER  // Folder browser for ROM download
} AppState;

static AppState currentState = STATE_LOADING;
static AppState previousState = STATE_PLATFORMS;  // For returning from settings
static bool needsConfigSetup = false;

// Shared state
static Config config;
static Platform *platforms = NULL;
static int platformCount = 0;
static int selectedPlatformIndex = 0;
static RomDetail *romDetail = NULL;
static int selectedRomIndex = 0;
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

// Helper to fetch platforms from API
static void fetch_platforms(void) {
    show_loading("Fetching platforms...");
    bottom_log("Fetching platforms...");
    if (platforms) {
        api_free_platforms(platforms, platformCount);
        platforms = NULL;
    }
    platforms = api_get_platforms(&platformCount);
    if (platforms) {
        bottom_log("Found %d platforms", platformCount);
        platforms_set_data(platforms, platformCount);
    } else {
        bottom_log("Failed to fetch platforms");
    }
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
        
        // Sync debug level from bottom screen to API
        api_set_debug_level(bottom_get_debug_level());
        
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
        
        // Handle download ROM from detail screen
        if (bottomAction == BOTTOM_ACTION_DOWNLOAD_ROM && currentState == STATE_ROM_DETAIL && romDetail) {
            // Check if we have a valid folder mapping for this platform
            const char *folderName = config_get_platform_folder(currentPlatformSlug);
            bool folderValid = false;
            
            if (folderName && folderName[0]) {
                // Check if the folder still exists
                char folderPath[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 2];
                snprintf(folderPath, sizeof(folderPath), "%s/%s", config.romFolder, folderName);
                struct stat st;
                folderValid = (stat(folderPath, &st) == 0 && S_ISDIR(st.st_mode));
            }
            
            if (folderValid) {
                // Folder exists - download directly
                char destPath[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 256 + 3];
                snprintf(destPath, sizeof(destPath), "%s/%s/%s", 
                        config.romFolder, folderName, romDetail->fileName);
                show_loading("Downloading ROM...");
                bottom_log("Downloading to: %s", destPath);
                if (api_download_rom(romDetail->id, destPath)) {
                    bottom_log("Download complete!");
                } else {
                    bottom_log("Download failed!");
                }
            } else {
                // No mapping or folder doesn't exist - show folder browser
                browser_init_rooted(config.romFolder, currentPlatformSlug);
                bottom_set_mode(BOTTOM_MODE_DEFAULT);
                currentState = STATE_SELECT_ROM_FOLDER;
            }
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
                        bottom_log("Configuration not valid. Please complete all fields.");
                    }
                }
                break;
            }
                
            case STATE_PLATFORMS: {
                PlatformsResult result = platforms_update(kDown, &selectedPlatformIndex);
                if (result == PLATFORMS_SELECTED && platforms && selectedPlatformIndex < platformCount) {
                    // Fetch ROMs for selected platform
                    show_loading("Fetching ROMs...");
                    bottom_log("Fetching ROMs for %s...", platforms[selectedPlatformIndex].displayName);
                    roms_clear();
                    int romCount, romTotal;
                    Rom *roms = api_get_roms(platforms[selectedPlatformIndex].id, 0, ROM_PAGE_SIZE, &romCount, &romTotal);
                    if (roms) {
                        bottom_log("Found %d/%d ROMs", romCount, romTotal);
                        roms_set_data(roms, romCount, romTotal, platforms[selectedPlatformIndex].displayName);
                        currentState = STATE_ROMS;
                    } else {
                        bottom_log("Failed to fetch ROMs");
                    }
                } else if (result == PLATFORMS_REFRESH) {
                    fetch_platforms();
                }
                break;
            }
                
            case STATE_ROMS: {
                RomsResult result = roms_update(kDown, &selectedRomIndex);
                if (result == ROMS_BACK) {
                    currentState = STATE_PLATFORMS;
                } else if (result == ROMS_SELECTED) {
                    // Fetch ROM details
                    int romId = roms_get_id_at(selectedRomIndex);
                    if (romId >= 0) {
                        show_loading("Loading ROM details...");
                        bottom_log("Fetching ROM details for ID %d...", romId);
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
                            bottom_set_mode(BOTTOM_MODE_ROM_DETAIL);
                            currentState = STATE_ROM_DETAIL;
                        } else {
                            bottom_log("Failed to fetch ROM details");
                        }
                    }
                } else if (result == ROMS_LOAD_MORE) {
                    // Fetch next page synchronously
                    show_loading("Loading more ROMs...");
                    int offset = roms_get_count();
                    int newCount, newTotal;
                    bottom_log("Loading more ROMs (offset %d)...", offset);
                    Rom *moreRoms = api_get_roms(platforms[selectedPlatformIndex].id, offset, ROM_PAGE_SIZE, &newCount, &newTotal);
                    if (moreRoms) {
                        bottom_log("Loaded %d more ROMs", newCount);
                        roms_append_data(moreRoms, newCount);
                    }
                }
                break;
            }
            
            case STATE_ROM_DETAIL: {
                RomDetailResult result = romdetail_update(kDown);
                if (result == ROMDETAIL_BACK) {
                    bottom_set_mode(BOTTOM_MODE_DEFAULT);
                    currentState = STATE_ROMS;
                }
                break;
            }
            
            case STATE_SELECT_ROM_FOLDER: {
                bool selected = browser_update(kDown);
                if (selected) {
                    // User selected a folder - save mapping and download
                    const char *folderName = browser_get_selected_folder_name();
                    config_set_platform_folder(&config, currentPlatformSlug, folderName);
                    browser_exit();
                    
                    // Now download
                    if (romDetail) {
                        char destPath[CONFIG_MAX_PATH_LEN + CONFIG_MAX_SLUG_LEN + 256 + 3];
                        snprintf(destPath, sizeof(destPath), "%s/%s/%s", 
                                config.romFolder, folderName, romDetail->fileName);
                        show_loading("Downloading ROM...");
                        bottom_log("Downloading to: %s", destPath);
                        if (api_download_rom(romDetail->id, destPath)) {
                            bottom_log("Download complete!");
                        } else {
                            bottom_log("Download failed!");
                        }
                    }
                    bottom_set_mode(BOTTOM_MODE_ROM_DETAIL);
                    currentState = STATE_ROM_DETAIL;
                } else if (browser_was_cancelled()) {
                    browser_exit();
                    bottom_set_mode(BOTTOM_MODE_ROM_DETAIL);
                    currentState = STATE_ROM_DETAIL;
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
