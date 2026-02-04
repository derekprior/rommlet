/*
 * Bottom screen - Toolbar and debug log modal
 */

#include "bottom.h"
#include "../ui.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// Log buffer configuration
#define LOG_MAX_LINES 100
#define LOG_LINE_LENGTH 64

// Layout constants for bottom screen
#define TOOLBAR_HEIGHT 24
#define ICON_SIZE 20
#define ICON_PADDING 4

// Touch zones
#define BUG_ICON_X (SCREEN_BOTTOM_WIDTH - ICON_SIZE - ICON_PADDING)
#define BUG_ICON_Y (ICON_PADDING)
#define CLOSE_ICON_X (SCREEN_BOTTOM_WIDTH - ICON_SIZE - ICON_PADDING)
#define CLOSE_ICON_Y (ICON_PADDING)

static bool showDebugModal = false;
static int debugLevel = 0;  // 0=off, 1=requests, 2=bodies
static int logScrollOffset = 0;

// Circular log buffer
static char logBuffer[LOG_MAX_LINES][LOG_LINE_LENGTH];
static int logHead = 0;  // Next write position
static int logCount = 0; // Number of lines stored

// Render target for bottom screen
static C3D_RenderTarget *bottomTarget = NULL;

void bottom_init(void) {
    bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    showDebugModal = false;
    debugLevel = 0;
    logScrollOffset = 0;
    logHead = 0;
    logCount = 0;
    
    // Clear log buffer
    memset(logBuffer, 0, sizeof(logBuffer));
    
    // Initial log message
    bottom_log("Rommlet - RomM Client");
    bottom_log("=====================");
}

void bottom_exit(void) {
    // Render target is cleaned up by citro2d
}

static bool touch_in_rect(int tx, int ty, int x, int y, int w, int h) {
    return tx >= x && tx < x + w && ty >= y && ty < y + h;
}

bool bottom_update(void) {
    // Handle touch input
    touchPosition touch;
    u32 kDown = hidKeysDown();
    
    if (kDown & KEY_TOUCH) {
        hidTouchRead(&touch);
        
        if (showDebugModal) {
            // Check for X (close) button tap
            if (touch_in_rect(touch.px, touch.py, CLOSE_ICON_X, CLOSE_ICON_Y, ICON_SIZE, ICON_SIZE)) {
                showDebugModal = false;
                return true;
            }
        } else {
            // Check for bug icon tap
            if (touch_in_rect(touch.px, touch.py, BUG_ICON_X, BUG_ICON_Y, ICON_SIZE, ICON_SIZE)) {
                showDebugModal = true;
                logScrollOffset = 0;
                return true;
            }
        }
    }
    
    // Handle debug level changes when modal is open
    if (showDebugModal) {
        if (kDown & KEY_ZR) {
            debugLevel = (debugLevel + 1) % 3;
            bottom_log("Debug level: %s", 
                debugLevel == 0 ? "OFF" : (debugLevel == 1 ? "REQUESTS" : "BODIES"));
            return true;
        }
        if (kDown & KEY_ZL) {
            debugLevel = (debugLevel - 1 + 3) % 3;
            bottom_log("Debug level: %s",
                debugLevel == 0 ? "OFF" : (debugLevel == 1 ? "REQUESTS" : "BODIES"));
            return true;
        }
        
        // Scroll log with C-stick (New 3DS only)
        circlePosition cstick;
        hidCstickRead(&cstick);
        if (cstick.dy > 40) {
            if (logScrollOffset > 0) logScrollOffset--;
        }
        if (cstick.dy < -40) {
            int maxScroll = logCount > 10 ? logCount - 10 : 0;
            if (logScrollOffset < maxScroll) logScrollOffset++;
        }
    }
    
    return false;
}

static void draw_toolbar(void) {
    // Header bar
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, TOOLBAR_HEIGHT, UI_COLOR_HEADER);
    
    // Bug icon (simple text representation)
    ui_draw_text(BUG_ICON_X + 2, BUG_ICON_Y + 2, "[?]", UI_COLOR_TEXT);
}

static void draw_debug_modal(void) {
    // Full background
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
    
    // Header
    ui_draw_header_bottom("Debug Log");
    
    // Close button (X)
    ui_draw_text(CLOSE_ICON_X + 4, CLOSE_ICON_Y + 2, "X", UI_COLOR_TEXT);
    
    // Debug level hint
    char levelHint[64];
    snprintf(levelHint, sizeof(levelHint), "ZL/ZR: Level (%s)",
        debugLevel == 0 ? "OFF" : (debugLevel == 1 ? "REQUESTS" : "BODIES"));
    ui_draw_text(UI_PADDING, UI_HEADER_HEIGHT + UI_PADDING, levelHint, UI_COLOR_TEXT_DIM);
    
    // Log content area
    float y = UI_HEADER_HEIGHT + UI_PADDING + UI_LINE_HEIGHT + UI_PADDING;
    int visibleLines = (SCREEN_BOTTOM_HEIGHT - y - UI_PADDING) / UI_LINE_HEIGHT;
    
    // Draw log lines
    for (int i = 0; i < visibleLines && i + logScrollOffset < logCount; i++) {
        int lineIndex = (logHead - logCount + i + logScrollOffset + LOG_MAX_LINES) % LOG_MAX_LINES;
        ui_draw_text(UI_PADDING, y, logBuffer[lineIndex], UI_COLOR_TEXT);
        y += UI_LINE_HEIGHT;
    }
    
    // Scroll indicator if needed
    if (logCount > visibleLines) {
        char scrollInfo[32];
        snprintf(scrollInfo, sizeof(scrollInfo), "%d/%d", logScrollOffset + 1, logCount);
        float textWidth = ui_get_text_width(scrollInfo);
        ui_draw_text(SCREEN_BOTTOM_WIDTH - textWidth - UI_PADDING,
                     UI_HEADER_HEIGHT + UI_PADDING, scrollInfo, UI_COLOR_TEXT_DIM);
    }
}

void bottom_draw(void) {
    C2D_SceneBegin(bottomTarget);
    C2D_TargetClear(bottomTarget, UI_COLOR_BG);
    
    if (showDebugModal) {
        draw_debug_modal();
    } else {
        draw_toolbar();
    }
}

void bottom_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    // Format the message
    vsnprintf(logBuffer[logHead], LOG_LINE_LENGTH, fmt, args);
    logBuffer[logHead][LOG_LINE_LENGTH - 1] = '\0';
    
    // Advance head
    logHead = (logHead + 1) % LOG_MAX_LINES;
    if (logCount < LOG_MAX_LINES) {
        logCount++;
    }
    
    va_end(args);
}

int bottom_get_debug_level(void) {
    return debugLevel;
}

void bottom_set_debug_level(int level) {
    debugLevel = level;
}
