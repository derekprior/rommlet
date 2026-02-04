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

// Touch scroll tracking
static int lastTouchY = -1;

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
    lastTouchY = -1;
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
    u32 kHeld = hidKeysHeld();
    
    if (kDown & KEY_TOUCH) {
        hidTouchRead(&touch);
        lastTouchY = touch.py;
        
        if (showDebugModal) {
            // Check for X (close) button tap
            if (touch_in_rect(touch.px, touch.py, CLOSE_ICON_X, CLOSE_ICON_Y, ICON_SIZE, ICON_SIZE)) {
                showDebugModal = false;
                lastTouchY = -1;
                return true;
            }
        } else {
            // Check for bug icon tap
            if (touch_in_rect(touch.px, touch.py, BUG_ICON_X, BUG_ICON_Y, ICON_SIZE, ICON_SIZE)) {
                showDebugModal = true;
                logScrollOffset = 0;
                lastTouchY = -1;
                return true;
            }
        }
    }
    
    // Handle touch drag scrolling in debug modal
    if (showDebugModal && (kHeld & KEY_TOUCH)) {
        hidTouchRead(&touch);
        
        // Check if touch is in the log content area
        if (touch.py >= logAreaTop && touch.py < logAreaTop + logAreaHeight) {
            if (lastTouchY >= 0) {
                int deltaY = lastTouchY - touch.py;
                if (deltaY != 0 && logCount > visibleLines) {
                    int maxScroll = logCount - visibleLines;
                    // Scroll by lines based on drag distance
                    int scrollAmount = deltaY / 10;
                    if (scrollAmount != 0) {
                        logScrollOffset += scrollAmount;
                        if (logScrollOffset < 0) logScrollOffset = 0;
                        if (logScrollOffset > maxScroll) logScrollOffset = maxScroll;
                        lastTouchY = touch.py;
                    }
                }
            } else {
                lastTouchY = touch.py;
            }
        }
    } else {
        lastTouchY = -1;
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
            int maxScroll = logCount > visibleLines ? logCount - visibleLines : 0;
            if (logScrollOffset < maxScroll) logScrollOffset++;
        }
    }
    
    return false;
}

// Draw a simple bug icon at the given position
static void draw_bug_icon(float x, float y, float size, u32 color) {
    float cx = x + size / 2;
    float cy = y + size / 2;
    float scale = size / 20.0f;  // Designed for 20px, scale accordingly
    
    // Body (oval) - draw as two overlapping circles
    float bodyW = 6 * scale;
    float bodyH = 8 * scale;
    C2D_DrawEllipseSolid(cx - bodyW/2, cy - bodyH/2 + 2*scale, 0, bodyW, bodyH, color);
    
    // Head (smaller circle)
    float headR = 3 * scale;
    C2D_DrawCircleSolid(cx, cy - 4*scale, 0, headR, color);
    
    // Antennae (two small lines using thin rectangles)
    C2D_DrawRectSolid(cx - 3*scale, cy - 7*scale, 0, 1*scale, 3*scale, color);
    C2D_DrawRectSolid(cx + 2*scale, cy - 7*scale, 0, 1*scale, 3*scale, color);
    
    // Legs (three pairs)
    // Left legs
    C2D_DrawRectSolid(cx - 6*scale, cy - 1*scale, 0, 4*scale, 1*scale, color);
    C2D_DrawRectSolid(cx - 6*scale, cy + 2*scale, 0, 4*scale, 1*scale, color);
    C2D_DrawRectSolid(cx - 5*scale, cy + 5*scale, 0, 3*scale, 1*scale, color);
    // Right legs
    C2D_DrawRectSolid(cx + 2*scale, cy - 1*scale, 0, 4*scale, 1*scale, color);
    C2D_DrawRectSolid(cx + 2*scale, cy + 2*scale, 0, 4*scale, 1*scale, color);
    C2D_DrawRectSolid(cx + 2*scale, cy + 5*scale, 0, 3*scale, 1*scale, color);
}

static void draw_toolbar(void) {
    // Header bar
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, TOOLBAR_HEIGHT, UI_COLOR_HEADER);
    
    // Bug icon
    draw_bug_icon(BUG_ICON_X, BUG_ICON_Y, ICON_SIZE, UI_COLOR_TEXT);
}

// Scrollbar dimensions
#define SCROLLBAR_WIDTH 16
#define SCROLLBAR_TRACK_COLOR C2D_Color32(0x3a, 0x3a, 0x50, 0xFF)
#define SCROLLBAR_THUMB_COLOR C2D_Color32(0x6a, 0x6a, 0x90, 0xFF)

// Log content area bounds (used for scrollbar calculations)
static float logAreaTop = 0;
static float logAreaHeight = 0;
static int visibleLines = 0;

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
    
    // Log content area (leave room for scrollbar)
    logAreaTop = UI_HEADER_HEIGHT + UI_PADDING + UI_LINE_HEIGHT + UI_PADDING;
    logAreaHeight = SCREEN_BOTTOM_HEIGHT - logAreaTop - UI_PADDING;
    visibleLines = (int)(logAreaHeight / UI_LINE_HEIGHT);
    
    float contentWidth = SCREEN_BOTTOM_WIDTH - UI_PADDING - SCROLLBAR_WIDTH - UI_PADDING;
    
    // Draw log lines
    float y = logAreaTop;
    for (int i = 0; i < visibleLines && i + logScrollOffset < logCount; i++) {
        int lineIndex = (logHead - logCount + i + logScrollOffset + LOG_MAX_LINES) % LOG_MAX_LINES;
        ui_draw_text(UI_PADDING, y, logBuffer[lineIndex], UI_COLOR_TEXT);
        y += UI_LINE_HEIGHT;
    }
    
    // Draw scrollbar track
    float scrollbarX = SCREEN_BOTTOM_WIDTH - SCROLLBAR_WIDTH - UI_PADDING;
    ui_draw_rect(scrollbarX, logAreaTop, SCROLLBAR_WIDTH, logAreaHeight, SCROLLBAR_TRACK_COLOR);
    
    // Draw scrollbar thumb if there's content to scroll
    if (logCount > visibleLines) {
        int maxScroll = logCount - visibleLines;
        float thumbHeight = (float)visibleLines / logCount * logAreaHeight;
        if (thumbHeight < 20) thumbHeight = 20; // Minimum thumb size
        
        float thumbY = logAreaTop + ((float)logScrollOffset / maxScroll) * (logAreaHeight - thumbHeight);
        ui_draw_rect(scrollbarX + 2, thumbY, SCROLLBAR_WIDTH - 4, thumbHeight, SCROLLBAR_THUMB_COLOR);
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
