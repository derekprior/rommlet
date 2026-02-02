/*
 * Rommlet - RomM Client for Nintendo 3DS
 * A homebrew app to browse ROMs on a RomM server
 * 
 * MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include <citro2d.h>
#include "config.h"
#include "api.h"
#include "ui.h"
#include "screens/settings.h"
#include "screens/platforms.h"
#include "screens/roms.h"

// App states
typedef enum {
    STATE_LOADING,
    STATE_SETTINGS,
    STATE_PLATFORMS,
    STATE_ROMS
} AppState;

static AppState currentState = STATE_LOADING;
static bool needsConfigSetup = false;

// Shared state
static Config config;
static Platform *platforms = NULL;
static int platformCount = 0;
static int selectedPlatformIndex = 0;
static Rom *roms = NULL;
static int romCount = 0;
static int romTotal = 0;

int main(int argc, char *argv[]) {
    // Initialize services
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    consoleInit(GFX_BOTTOM, NULL);
    
    // Initialize render targets
    C3D_RenderTarget *topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    
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
        currentState = STATE_SETTINGS;
    } else {
        // Set up API with loaded config
        api_set_base_url(config.serverUrl);
        api_set_auth(config.username, config.password);
        currentState = STATE_PLATFORMS;
    }
    
    // Initialize screens
    settings_init(&config);
    platforms_init();
    roms_init();
    
    printf("\x1b[1;1H\x1b[2J"); // Clear console
    printf("Rommlet - RomM Client\n");
    printf("=====================\n\n");
    printf("L/R: Debug level (currently: OFF)\n\n");
    
    // Main loop
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        
        // Global exit
        if (kDown & KEY_START) {
            break;
        }
        
        // Global debug level controls
        if (kDown & KEY_ZR) {
            int level = api_get_debug_level();
            level = (level + 1) % 3;  // Cycle 0 -> 1 -> 2 -> 0
            api_set_debug_level(level);
            printf("\x1b[4;1H\x1b[2K");  // Move to line 4, clear it
            printf("ZL/ZR: Debug level (currently: %s)\n",
                   level == 0 ? "OFF" : (level == 1 ? "REQUESTS" : "BODIES"));
        }
        if (kDown & KEY_ZL) {
            int level = api_get_debug_level();
            level = (level + 2) % 3;  // Cycle 0 -> 2 -> 1 -> 0
            api_set_debug_level(level);
            printf("\x1b[4;1H\x1b[2K");  // Move to line 4, clear it
            printf("ZL/ZR: Debug level (currently: %s)\n",
                   level == 0 ? "OFF" : (level == 1 ? "REQUESTS" : "BODIES"));
        }
        
        // Handle state-specific input and updates
        switch (currentState) {
            case STATE_LOADING:
                // Transition to appropriate screen
                if (needsConfigSetup) {
                    currentState = STATE_SETTINGS;
                } else {
                    currentState = STATE_PLATFORMS;
                    
                    // Fetch platforms on startup
                    printf("Fetching platforms...\n");
                    platforms = api_get_platforms(&platformCount);
                    if (platforms) {
                        printf("Found %d platforms\n", platformCount);
                        platforms_set_data(platforms, platformCount);
                    } else {
                        printf("Failed to fetch platforms\n");
                    }
                }
                break;
                
            case STATE_SETTINGS: {
                SettingsResult result = settings_update(kDown);
                if (result == SETTINGS_SAVED) {
                    config_save(&config);
                    api_set_auth(config.username, config.password);
                    api_set_base_url(config.serverUrl);
                    currentState = STATE_PLATFORMS;
                    
                    // Fetch platforms
                    printf("Fetching platforms...\n");
                    if (platforms) {
                        api_free_platforms(platforms, platformCount);
                        platforms = NULL;
                    }
                    platforms = api_get_platforms(&platformCount);
                    if (platforms) {
                        printf("Found %d platforms\n", platformCount);
                        platforms_set_data(platforms, platformCount);
                    } else {
                        printf("Failed to fetch platforms\n");
                    }
                }
                break;
            }
                
            case STATE_PLATFORMS: {
                PlatformsResult result = platforms_update(kDown, &selectedPlatformIndex);
                if (result == PLATFORMS_SELECTED && platforms && selectedPlatformIndex < platformCount) {
                    // Fetch ROMs for selected platform
                    printf("Fetching ROMs for %s...\n", platforms[selectedPlatformIndex].displayName);
                    if (roms) {
                        api_free_roms(roms, romCount);
                        roms = NULL;
                    }
                    roms = api_get_roms(platforms[selectedPlatformIndex].id, 0, 20, &romCount, &romTotal);
                    if (roms) {
                        printf("Found %d/%d ROMs\n", romCount, romTotal);
                        roms_set_data(roms, romCount, romTotal, platforms[selectedPlatformIndex].displayName);
                        currentState = STATE_ROMS;
                    } else {
                        printf("Failed to fetch ROMs\n");
                    }
                } else if (result == PLATFORMS_SETTINGS) {
                    currentState = STATE_SETTINGS;
                } else if (result == PLATFORMS_REFRESH) {
                    // Refresh platforms
                    printf("Refreshing platforms...\n");
                    if (platforms) {
                        api_free_platforms(platforms, platformCount);
                        platforms = NULL;
                    }
                    platforms = api_get_platforms(&platformCount);
                    if (platforms) {
                        printf("Found %d platforms\n", platformCount);
                        platforms_set_data(platforms, platformCount);
                    }
                }
                break;
            }
                
            case STATE_ROMS: {
                RomsResult result = roms_update(kDown);
                if (result == ROMS_BACK) {
                    currentState = STATE_PLATFORMS;
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
        }
        
        C3D_FrameEnd(0);
        
        // Bottom screen controls help (via console)
        // Already handled by printf in each state
    }
    
    // Cleanup
    if (platforms) api_free_platforms(platforms, platformCount);
    if (roms) api_free_roms(roms, romCount);
    
    ui_exit();
    api_exit();
    
    httpcExit();
    romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    
    return 0;
}
