/*
 * Bottom screen - Toolbar and debug log modal
 */

#include "bottom.h"
#include "../ui.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

// Log buffer configuration
#define LOG_MAX_LINES 100
#define LOG_LINE_LENGTH 64

// Layout constants for bottom screen
#define TOOLBAR_HEIGHT 24
#define ICON_SIZE 20
#define ICON_PADDING 4

// Touch zones - bug icon on right, gear icon next to it
#define BUG_ICON_X (SCREEN_BOTTOM_WIDTH - ICON_SIZE - ICON_PADDING)
#define BUG_ICON_Y (ICON_PADDING)
#define GEAR_ICON_X (SCREEN_BOTTOM_WIDTH - (ICON_SIZE + ICON_PADDING) * 2)
#define GEAR_ICON_Y (ICON_PADDING)
#define CLOSE_ICON_X (SCREEN_BOTTOM_WIDTH - ICON_SIZE - ICON_PADDING)
#define CLOSE_ICON_Y (ICON_PADDING)

static bool showDebugModal = false;
static int debugLevel = 0;  // 0=off, 1=requests, 2=bodies
static int logScrollOffset = 0;
static BottomMode currentMode = BOTTOM_MODE_DEFAULT;

// Touch scroll tracking
static int lastTouchY = -1;

// Button press state for visual feedback
static bool saveButtonPressed = false;
static bool cancelButtonPressed = false;
static bool showCancelButton = false;  // Only show if config was valid before editing

// Circular log buffer
static char logBuffer[LOG_MAX_LINES][LOG_LINE_LENGTH];
static int logHead = 0;  // Next write position
static int logCount = 0; // Number of lines stored

// Render target for bottom screen
static C3D_RenderTarget *bottomTarget = NULL;

// Log content area bounds (used for scrollbar calculations)
static float logAreaTop = 0;
static float logAreaHeight = 0;
static int visibleLines = 0;

// Button dimensions for settings screen
#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 50
#define BUTTON_SPACING 15
// When cancel is shown, both buttons are vertically centered
#define SAVE_BUTTON_Y_SINGLE ((SCREEN_BOTTOM_HEIGHT - BUTTON_HEIGHT) / 2)
#define SAVE_BUTTON_Y_DUAL ((SCREEN_BOTTOM_HEIGHT - BUTTON_HEIGHT * 2 - BUTTON_SPACING) / 2)
#define CANCEL_BUTTON_Y (SAVE_BUTTON_Y_DUAL + BUTTON_HEIGHT + BUTTON_SPACING)
#define BUTTON_X ((SCREEN_BOTTOM_WIDTH - BUTTON_WIDTH) / 2)

// Primary button colors (green)
#define BUTTON_COLOR_TOP     C2D_Color32(0x5a, 0xa0, 0x5a, 0xFF)
#define BUTTON_COLOR_BOTTOM  C2D_Color32(0x3a, 0x80, 0x3a, 0xFF)
#define BUTTON_COLOR_PRESSED C2D_Color32(0x2a, 0x60, 0x2a, 0xFF)
#define BUTTON_HIGHLIGHT     C2D_Color32(0x7a, 0xc0, 0x7a, 0xFF)
#define BUTTON_BORDER        C2D_Color32(0x2a, 0x50, 0x2a, 0xFF)

// Secondary button colors (gray)
#define BUTTON2_COLOR_TOP     C2D_Color32(0x6a, 0x6a, 0x70, 0xFF)
#define BUTTON2_COLOR_BOTTOM  C2D_Color32(0x50, 0x50, 0x56, 0xFF)
#define BUTTON2_COLOR_PRESSED C2D_Color32(0x40, 0x40, 0x46, 0xFF)
#define BUTTON2_HIGHLIGHT     C2D_Color32(0x8a, 0x8a, 0x90, 0xFF)
#define BUTTON2_BORDER        C2D_Color32(0x3a, 0x3a, 0x40, 0xFF)

// Shared
#define BUTTON_SHADOW_COLOR  C2D_Color32(0x1a, 0x1a, 0x2e, 0x80)

void bottom_init(void) {
    bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    showDebugModal = false;
    debugLevel = 0;
    logScrollOffset = 0;
    lastTouchY = -1;
    currentMode = BOTTOM_MODE_DEFAULT;
    showCancelButton = false;
    saveButtonPressed = false;
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

void bottom_set_mode(BottomMode mode) {
    currentMode = mode;
    saveButtonPressed = false;
    cancelButtonPressed = false;
    if (mode != BOTTOM_MODE_SETTINGS) {
        showCancelButton = false;
    }
}

void bottom_set_settings_mode(bool canCancel) {
    currentMode = BOTTOM_MODE_SETTINGS;
    saveButtonPressed = false;
    cancelButtonPressed = false;
    showCancelButton = canCancel;
}

static bool touch_in_rect(int tx, int ty, int x, int y, int w, int h) {
    return tx >= x && tx < x + w && ty >= y && ty < y + h;
}

BottomAction bottom_update(void) {
    // Handle touch input
    touchPosition touch;
    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();
    u32 kUp = hidKeysUp();
    
    BottomAction action = BOTTOM_ACTION_NONE;
    
    // Handle settings mode buttons
    if (currentMode == BOTTOM_MODE_SETTINGS && !showDebugModal) {
        float saveButtonY = showCancelButton ? SAVE_BUTTON_Y_DUAL : SAVE_BUTTON_Y_SINGLE;
        
        if (kDown & KEY_TOUCH) {
            hidTouchRead(&touch);
            if (touch_in_rect(touch.px, touch.py, BUTTON_X, saveButtonY, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                saveButtonPressed = true;
            }
            if (showCancelButton && touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                cancelButtonPressed = true;
            }
        }
        if (kHeld & KEY_TOUCH) {
            hidTouchRead(&touch);
            saveButtonPressed = touch_in_rect(touch.px, touch.py, BUTTON_X, saveButtonY, BUTTON_WIDTH, BUTTON_HEIGHT);
            if (showCancelButton) {
                cancelButtonPressed = touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
            }
        }
        if (kUp & KEY_TOUCH) {
            if (saveButtonPressed) {
                action = BOTTOM_ACTION_SAVE_SETTINGS;
            }
            if (cancelButtonPressed) {
                action = BOTTOM_ACTION_CANCEL_SETTINGS;
            }
            saveButtonPressed = false;
            cancelButtonPressed = false;
        }
    }
    
    if (kDown & KEY_TOUCH) {
        hidTouchRead(&touch);
        lastTouchY = touch.py;
        
        if (showDebugModal) {
            // Check for X (close) button tap
            if (touch_in_rect(touch.px, touch.py, CLOSE_ICON_X, CLOSE_ICON_Y, ICON_SIZE, ICON_SIZE)) {
                showDebugModal = false;
                lastTouchY = -1;
                return action;
            }
        } else {
            // Check for bug icon tap
            if (touch_in_rect(touch.px, touch.py, BUG_ICON_X, BUG_ICON_Y, ICON_SIZE, ICON_SIZE)) {
                showDebugModal = true;
                logScrollOffset = 0;
                lastTouchY = -1;
                return action;
            }
            // Check for gear icon tap (only if not already on settings)
            if (currentMode != BOTTOM_MODE_SETTINGS &&
                touch_in_rect(touch.px, touch.py, GEAR_ICON_X, GEAR_ICON_Y, ICON_SIZE, ICON_SIZE)) {
                return BOTTOM_ACTION_OPEN_SETTINGS;
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
    } else if (!saveButtonPressed) {
        lastTouchY = -1;
    }
    
    // Handle debug level changes when modal is open
    if (showDebugModal) {
        if (kDown & KEY_ZR) {
            debugLevel = (debugLevel + 1) % 3;
            bottom_log("Debug level: %s", 
                debugLevel == 0 ? "OFF" : (debugLevel == 1 ? "REQUESTS" : "BODIES"));
            return action;
        }
        if (kDown & KEY_ZL) {
            debugLevel = (debugLevel - 1 + 3) % 3;
            bottom_log("Debug level: %s",
                debugLevel == 0 ? "OFF" : (debugLevel == 1 ? "REQUESTS" : "BODIES"));
            return action;
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
    
    return action;
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

// Draw a gear/settings icon at the given position
static void draw_gear_icon(float x, float y, float size, u32 color) {
    float cx = x + size / 2;
    float cy = y + size / 2;
    float scale = size / 20.0f;
    
    // Center circle (the hole)
    float innerR = 3 * scale;
    float outerR = 6 * scale;
    
    // Draw outer gear body
    C2D_DrawCircleSolid(cx, cy, 0, outerR, color);
    
    // Draw teeth (8 teeth around the gear)
    float toothLen = 3 * scale;
    float toothWidth = 2.5 * scale;
    for (int i = 0; i < 8; i++) {
        float angle = i * 3.14159f / 4.0f;
        
        // Draw tooth as small rectangle at gear edge
        float toothCx = cx + (outerR + toothLen/2 - 2) * cosf(angle);
        float toothCy = cy + (outerR + toothLen/2 - 2) * sinf(angle);
        C2D_DrawRectSolid(toothCx - toothWidth/2, toothCy - toothWidth/2, 0, toothWidth, toothWidth, color);
    }
    
    // Draw inner circle (hole) with background color
    C2D_DrawCircleSolid(cx, cy, 0, innerR, UI_COLOR_HEADER);
}

static void draw_toolbar(void) {
    // Header bar
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, TOOLBAR_HEIGHT, UI_COLOR_HEADER);
    
    // Gear icon (settings)
    draw_gear_icon(GEAR_ICON_X, GEAR_ICON_Y, ICON_SIZE, UI_COLOR_TEXT);
    
    // Bug icon (debug)
    draw_bug_icon(BUG_ICON_X, BUG_ICON_Y, ICON_SIZE, UI_COLOR_TEXT);
}

// Scrollbar dimensions
#define SCROLLBAR_WIDTH 16

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
    
    // Draw log lines
    float y = logAreaTop;
    for (int i = 0; i < visibleLines && i + logScrollOffset < logCount; i++) {
        int lineIndex = (logHead - logCount + i + logScrollOffset + LOG_MAX_LINES) % LOG_MAX_LINES;
        ui_draw_text(UI_PADDING, y, logBuffer[lineIndex], UI_COLOR_TEXT);
        y += UI_LINE_HEIGHT;
    }
    
    // Draw scrollbar track
    float scrollbarX = SCREEN_BOTTOM_WIDTH - SCROLLBAR_WIDTH - UI_PADDING;
    ui_draw_rect(scrollbarX, logAreaTop, SCROLLBAR_WIDTH, logAreaHeight, UI_COLOR_SCROLLBAR_TRACK);
    
    // Draw scrollbar thumb if there's content to scroll
    if (logCount > visibleLines) {
        int maxScroll = logCount - visibleLines;
        float thumbHeight = (float)visibleLines / logCount * logAreaHeight;
        if (thumbHeight < 20) thumbHeight = 20; // Minimum thumb size
        
        float thumbY = logAreaTop + ((float)logScrollOffset / maxScroll) * (logAreaHeight - thumbHeight);
        ui_draw_rect(scrollbarX + 2, thumbY, SCROLLBAR_WIDTH - 4, thumbHeight, UI_COLOR_SCROLLBAR_THUMB);
    }
}

// Draw a 3DS-style button with shadow and gradient effect
static void draw_button(float x, float y, float w, float h, const char *text, bool pressed, bool secondary) {
    // Shadow (offset down and right)
    if (!pressed) {
        ui_draw_rect(x + 3, y + 3, w, h, BUTTON_SHADOW_COLOR);
    }
    
    // Button position shifts down when pressed
    float bx = pressed ? x + 1 : x;
    float by = pressed ? y + 1 : y;
    
    // Colors based on style
    u32 colorTop = secondary ? BUTTON2_COLOR_TOP : BUTTON_COLOR_TOP;
    u32 colorBottom = secondary ? BUTTON2_COLOR_BOTTOM : BUTTON_COLOR_BOTTOM;
    u32 colorPressed = secondary ? BUTTON2_COLOR_PRESSED : BUTTON_COLOR_PRESSED;
    u32 colorHighlight = secondary ? BUTTON2_HIGHLIGHT : BUTTON_HIGHLIGHT;
    u32 colorBorder = secondary ? BUTTON2_BORDER : BUTTON_BORDER;
    
    // Dark border
    ui_draw_rect(bx - 2, by - 2, w + 4, h + 4, colorBorder);
    
    // Main button body (use pressed color or gradient colors)
    if (pressed) {
        ui_draw_rect(bx, by, w, h, colorPressed);
    } else {
        // Top half (lighter)
        ui_draw_rect(bx, by, w, h / 2, colorTop);
        // Bottom half (darker)
        ui_draw_rect(bx, by + h / 2, w, h / 2, colorBottom);
        // Top highlight line
        ui_draw_rect(bx, by, w, 2, colorHighlight);
    }
    
    // Center the text
    float textWidth = ui_get_text_width(text);
    float textX = bx + (w - textWidth) / 2;
    float textY = by + (h - 16) / 2;  // Approximate text height
    ui_draw_text(textX, textY, text, UI_COLOR_TEXT);
}

static void draw_settings_screen(void) {
    // Background
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
    
    // Toolbar at top with bug icon
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, TOOLBAR_HEIGHT, UI_COLOR_HEADER);
    draw_bug_icon(BUG_ICON_X, BUG_ICON_Y, ICON_SIZE, UI_COLOR_TEXT);
    
    // Buttons - position depends on whether cancel is shown
    if (showCancelButton) {
        draw_button(BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT, "Save and Connect", saveButtonPressed, false);
        draw_button(BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, "Cancel", cancelButtonPressed, true);
    } else {
        draw_button(BUTTON_X, SAVE_BUTTON_Y_SINGLE, BUTTON_WIDTH, BUTTON_HEIGHT, "Save and Connect", saveButtonPressed, false);
    }
}

void bottom_draw(void) {
    C2D_SceneBegin(bottomTarget);
    C2D_TargetClear(bottomTarget, UI_COLOR_BG);
    
    if (showDebugModal) {
        draw_debug_modal();
    } else if (currentMode == BOTTOM_MODE_SETTINGS) {
        draw_settings_screen();
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
