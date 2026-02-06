/*
 * Bottom screen - Toolbar and debug log modal
 */

#include "bottom.h"
#include "search.h"
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

// Touch zones - bug icon on right, gear icon next to it, queue icon next
#define BUG_ICON_X (SCREEN_BOTTOM_WIDTH - ICON_SIZE - ICON_PADDING)
#define BUG_ICON_Y (ICON_PADDING)
#define GEAR_ICON_X (SCREEN_BOTTOM_WIDTH - (ICON_SIZE + ICON_PADDING) * 2)
#define GEAR_ICON_Y (ICON_PADDING)
#define QUEUE_ICON_X (SCREEN_BOTTOM_WIDTH - (ICON_SIZE + ICON_PADDING) * 3)
#define QUEUE_ICON_Y (ICON_PADDING)
#define SEARCH_ICON_X (ICON_PADDING)
#define SEARCH_ICON_Y (ICON_PADDING)
#define CLOSE_ICON_X (SCREEN_BOTTOM_WIDTH - ICON_SIZE - ICON_PADDING)
#define CLOSE_ICON_Y (ICON_PADDING)

static bool showDebugModal = false;
static int logScrollOffset = 0;
static int logScrollOffsetX = 0;
static BottomMode currentMode = BOTTOM_MODE_DEFAULT;

// Touch scroll tracking
static int lastTouchY = -1;

// Button press state for visual feedback
static bool saveButtonPressed = false;
static bool cancelButtonPressed = false;
static bool downloadButtonPressed = false;
static bool queueButtonPressed = false;
static bool startDownloadsPressed = false;
static bool clearQueuePressed = false;
static bool confirmClearPressed = false;
static bool cancelClearPressed = false;
static bool romExists = false;
static bool romQueued = false;
static int queueItemCount = 0;
static bool searchButtonPressed = false;
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

// Danger button colors (red)
#define BUTTON_DANGER_COLOR_TOP     C2D_Color32(0xc0, 0x40, 0x40, 0xFF)
#define BUTTON_DANGER_COLOR_BOTTOM  C2D_Color32(0xa0, 0x30, 0x30, 0xFF)
#define BUTTON_DANGER_COLOR_PRESSED C2D_Color32(0x80, 0x20, 0x20, 0xFF)
#define BUTTON_DANGER_HIGHLIGHT     C2D_Color32(0xe0, 0x60, 0x60, 0xFF)
#define BUTTON_DANGER_BORDER        C2D_Color32(0x60, 0x20, 0x20, 0xFF)

// Shared
#define BUTTON_SHADOW_COLOR  C2D_Color32(0x1a, 0x1a, 0x2e, 0x80)

void bottom_init(void) {
    bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    showDebugModal = false;
    logScrollOffset = 0;
    logScrollOffsetX = 0;
    lastTouchY = -1;
    currentMode = BOTTOM_MODE_DEFAULT;
    showCancelButton = false;
    saveButtonPressed = false;
    logHead = 0;
    logCount = 0;
    
    // Clear log buffer
    memset(logBuffer, 0, sizeof(logBuffer));
}

void bottom_exit(void) {
    // Render target is cleaned up by citro2d
}

void bottom_set_mode(BottomMode mode) {
    currentMode = mode;
    saveButtonPressed = false;
    cancelButtonPressed = false;
    downloadButtonPressed = false;
    queueButtonPressed = false;
    startDownloadsPressed = false;
    clearQueuePressed = false;
    confirmClearPressed = false;
    cancelClearPressed = false;
    searchButtonPressed = false;
    if (mode != BOTTOM_MODE_SETTINGS) {
        showCancelButton = false;
    }
}

void bottom_set_settings_mode(bool canCancel) {
    currentMode = BOTTOM_MODE_SETTINGS;
    saveButtonPressed = false;
    cancelButtonPressed = false;
    downloadButtonPressed = false;
    queueButtonPressed = false;
    showCancelButton = canCancel;
}

void bottom_set_rom_exists(bool exists) {
    romExists = exists;
}

void bottom_set_rom_queued(bool queued) {
    romQueued = queued;
}

void bottom_set_queue_count(int count) {
    queueItemCount = count;
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
    
    // Handle ROM detail mode buttons (download + queue)
    if ((currentMode == BOTTOM_MODE_ROM_DETAIL || currentMode == BOTTOM_MODE_ROMS || currentMode == BOTTOM_MODE_SEARCH_RESULTS) && !showDebugModal) {
        if (kDown & KEY_TOUCH) {
            hidTouchRead(&touch);
            if (touch_in_rect(touch.px, touch.py, BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                downloadButtonPressed = true;
            }
            if (touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                queueButtonPressed = true;
            }
        }
        if (kHeld & KEY_TOUCH) {
            hidTouchRead(&touch);
            downloadButtonPressed = touch_in_rect(touch.px, touch.py, BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT);
            queueButtonPressed = touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
        }
        if (kUp & KEY_TOUCH) {
            if (downloadButtonPressed) {
                action = BOTTOM_ACTION_DOWNLOAD_ROM;
            }
            if (queueButtonPressed) {
                action = BOTTOM_ACTION_QUEUE_ROM;
            }
            downloadButtonPressed = false;
            queueButtonPressed = false;
        }
    }
    
    // Handle queue mode buttons (start downloads + clear queue)
    if (currentMode == BOTTOM_MODE_QUEUE && !showDebugModal) {
        if (kDown & KEY_TOUCH) {
            hidTouchRead(&touch);
            if (touch_in_rect(touch.px, touch.py, BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                startDownloadsPressed = true;
            }
            if (queueItemCount > 0 && touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                clearQueuePressed = true;
            }
        }
        if (kHeld & KEY_TOUCH) {
            hidTouchRead(&touch);
            startDownloadsPressed = touch_in_rect(touch.px, touch.py, BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT);
            if (queueItemCount > 0) {
                clearQueuePressed = touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
            }
        }
        if (kUp & KEY_TOUCH) {
            if (startDownloadsPressed) {
                action = BOTTOM_ACTION_START_DOWNLOADS;
            }
            if (clearQueuePressed) {
                action = BOTTOM_ACTION_CLEAR_QUEUE;
            }
            startDownloadsPressed = false;
            clearQueuePressed = false;
        }
    }
    
    // Handle queue confirm clear dialog
    if (currentMode == BOTTOM_MODE_QUEUE_CONFIRM && !showDebugModal) {
        if (kDown & KEY_TOUCH) {
            hidTouchRead(&touch);
            if (touch_in_rect(touch.px, touch.py, BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                confirmClearPressed = true;
            }
            if (touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                cancelClearPressed = true;
            }
        }
        if (kHeld & KEY_TOUCH) {
            hidTouchRead(&touch);
            confirmClearPressed = touch_in_rect(touch.px, touch.py, BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT);
            cancelClearPressed = touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
        }
        if (kUp & KEY_TOUCH) {
            if (confirmClearPressed) {
                action = BOTTOM_ACTION_CLEAR_QUEUE;
            }
            if (cancelClearPressed) {
                action = BOTTOM_ACTION_CANCEL_CLEAR;
            }
            confirmClearPressed = false;
            cancelClearPressed = false;
        }
    }
    
    // Handle search form mode (search field tap + search button)
    if (currentMode == BOTTOM_MODE_SEARCH_FORM && !showDebugModal) {
        // Search field area: top area below toolbar
        float fieldY = 24 + UI_PADDING;  // TOOLBAR_HEIGHT + padding
        float fieldH = 22;
        // Search button area: bottom of screen
        float btnY = SCREEN_BOTTOM_HEIGHT - 30 - UI_PADDING;
        float btnH = 30;
        float btnX = (SCREEN_BOTTOM_WIDTH - 200) / 2;
        float btnW = 200;
        
        if (kDown & KEY_TOUCH) {
            hidTouchRead(&touch);
            if (touch_in_rect(touch.px, touch.py, UI_PADDING, fieldY, SCREEN_BOTTOM_WIDTH - UI_PADDING * 2, fieldH)) {
                action = BOTTOM_ACTION_SEARCH_FIELD;
            }
            if (touch_in_rect(touch.px, touch.py, btnX, btnY, btnW, btnH)) {
                searchButtonPressed = true;
            }
        }
        if (kHeld & KEY_TOUCH) {
            hidTouchRead(&touch);
            searchButtonPressed = touch_in_rect(touch.px, touch.py, btnX, btnY, btnW, btnH);
        }
        if (kUp & KEY_TOUCH) {
            if (searchButtonPressed) {
                action = BOTTOM_ACTION_SEARCH_EXECUTE;
            }
            searchButtonPressed = false;
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
                logScrollOffsetX = 0;
                lastTouchY = -1;
                return action;
            }
            // Check for gear icon tap (only if not already on settings)
            if (currentMode != BOTTOM_MODE_SETTINGS &&
                touch_in_rect(touch.px, touch.py, GEAR_ICON_X, GEAR_ICON_Y, ICON_SIZE, ICON_SIZE)) {
                return BOTTOM_ACTION_OPEN_SETTINGS;
            }
            // Check for queue icon tap
            if (currentMode != BOTTOM_MODE_QUEUE && currentMode != BOTTOM_MODE_QUEUE_CONFIRM &&
                touch_in_rect(touch.px, touch.py, QUEUE_ICON_X, QUEUE_ICON_Y, ICON_SIZE, ICON_SIZE)) {
                return BOTTOM_ACTION_OPEN_QUEUE;
            }
            // Check for search icon tap
            if (currentMode != BOTTOM_MODE_SEARCH_FORM &&
                touch_in_rect(touch.px, touch.py, SEARCH_ICON_X, SEARCH_ICON_Y, ICON_SIZE, ICON_SIZE)) {
                return BOTTOM_ACTION_OPEN_SEARCH;
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
    
    // Handle log level changes when modal is open
    if (showDebugModal) {
        if (kDown & KEY_ZR) {
            // Cycle through: INFO -> DEBUG -> TRACE -> INFO
            LogLevel current = log_get_level();
            if (current == LOG_INFO) {
                log_set_level(LOG_DEBUG);
            } else if (current == LOG_DEBUG) {
                log_set_level(LOG_TRACE);
            } else {
                log_set_level(LOG_INFO);
            }
            log_info("Log level: %s", log_level_name(log_get_level()));
            return action;
        }
        if (kDown & KEY_ZL) {
            // Cycle backwards: INFO -> TRACE -> DEBUG -> INFO
            LogLevel current = log_get_level();
            if (current == LOG_INFO) {
                log_set_level(LOG_TRACE);
            } else if (current == LOG_TRACE) {
                log_set_level(LOG_DEBUG);
            } else {
                log_set_level(LOG_INFO);
            }
            log_info("Log level: %s", log_level_name(log_get_level()));
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
        if (cstick.dx > 40) {
            logScrollOffsetX += 4;
            int maxScrollX = LOG_LINE_LENGTH * 8;
            if (logScrollOffsetX > maxScrollX) logScrollOffsetX = maxScrollX;
        }
        if (cstick.dx < -40) {
            logScrollOffsetX -= 4;
            if (logScrollOffsetX < 0) logScrollOffsetX = 0;
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

// Draw a queue/list icon at the given position
static void draw_queue_icon(float x, float y, float size, u32 color) {
    float scale = size / 20.0f;
    float lx = x + 3 * scale;
    float ly = y + 4 * scale;
    float lineW = 14 * scale;
    float lineH = 2 * scale;
    float gap = 4 * scale;

    // Three horizontal lines (list icon)
    C2D_DrawRectSolid(lx, ly, 0, lineW, lineH, color);
    C2D_DrawRectSolid(lx, ly + gap, 0, lineW, lineH, color);
    C2D_DrawRectSolid(lx, ly + gap * 2, 0, lineW, lineH, color);

    // Small arrow pointing down on right side
    float ax = x + 14 * scale;
    float ay = ly + gap * 2 + lineH + 1 * scale;
    C2D_DrawRectSolid(ax, ay, 0, 3 * scale, 2 * scale, color);
}

// Draw a magnifying glass icon at the given position
static void draw_search_icon(float x, float y, float size, u32 color) {
    float cx = x + size / 2;
    float cy = y + size / 2;
    float scale = size / 20.0f;
    
    // Lens circle
    float lensR = 5 * scale;
    float lensCx = cx - 2 * scale;
    float lensCy = cy - 2 * scale;
    C2D_DrawCircleSolid(lensCx, lensCy, 0, lensR, color);
    C2D_DrawCircleSolid(lensCx, lensCy, 0, lensR - 2 * scale, UI_COLOR_HEADER);
    
    // Handle at ~45 degrees (series of small squares along diagonal)
    float hw = 2.5f * scale;
    for (int i = 0; i < 4; i++) {
        float offset = (3 + i * 1.5f) * scale;
        C2D_DrawRectSolid(lensCx + offset - hw/2, lensCy + offset - hw/2, 0, hw, hw, color);
    }
}

static void draw_toolbar(void) {
    // Header bar
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, TOOLBAR_HEIGHT, UI_COLOR_HEADER);
    
    // Queue icon
    draw_queue_icon(QUEUE_ICON_X, QUEUE_ICON_Y, ICON_SIZE, UI_COLOR_TEXT);
    
    // Gear icon (settings)
    draw_gear_icon(GEAR_ICON_X, GEAR_ICON_Y, ICON_SIZE, UI_COLOR_TEXT);
    
    // Bug icon (debug)
    draw_bug_icon(BUG_ICON_X, BUG_ICON_Y, ICON_SIZE, UI_COLOR_TEXT);
    
    // Search icon
    draw_search_icon(SEARCH_ICON_X, SEARCH_ICON_Y, ICON_SIZE, UI_COLOR_TEXT);
}

static void draw_debug_modal(void) {
    // Full background
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
    
    // Header
    ui_draw_header_bottom("Debug Log");
    
    // Close button (X)
    ui_draw_text(CLOSE_ICON_X + 4, CLOSE_ICON_Y + 2, "X", UI_COLOR_TEXT);
    
    // Log level hint
    char levelHint[64];
    snprintf(levelHint, sizeof(levelHint), "ZL/ZR: Level (%s)", log_level_name(log_get_level()));
    ui_draw_text(UI_PADDING, UI_HEADER_HEIGHT + UI_PADDING, levelHint, UI_COLOR_TEXT_DIM);
    
    // Log content area
    logAreaTop = UI_HEADER_HEIGHT + UI_PADDING + UI_LINE_HEIGHT + UI_PADDING;
    logAreaHeight = SCREEN_BOTTOM_HEIGHT - logAreaTop - UI_PADDING;
    visibleLines = (int)(logAreaHeight / UI_LINE_HEIGHT);
    
    // Draw log lines with horizontal offset
    float y = logAreaTop;
    for (int i = 0; i < visibleLines && i + logScrollOffset < logCount; i++) {
        int lineIndex = (logHead - logCount + i + logScrollOffset + LOG_MAX_LINES) % LOG_MAX_LINES;
        ui_draw_text(UI_PADDING - logScrollOffsetX, y, logBuffer[lineIndex], UI_COLOR_TEXT);
        y += UI_LINE_HEIGHT;
    }
}

// Draw a 3DS-style button with shadow and gradient effect
typedef enum { BUTTON_STYLE_PRIMARY, BUTTON_STYLE_SECONDARY, BUTTON_STYLE_DANGER } ButtonStyle;

static void draw_button(float x, float y, float w, float h, const char *text, bool pressed, ButtonStyle style) {
    // Shadow (offset down and right)
    if (!pressed) {
        ui_draw_rect(x + 3, y + 3, w, h, BUTTON_SHADOW_COLOR);
    }
    
    // Button position shifts down when pressed
    float bx = pressed ? x + 1 : x;
    float by = pressed ? y + 1 : y;
    
    // Colors based on style
    u32 colorTop, colorBottom, colorPressed, colorHighlight, colorBorder;
    switch (style) {
        case BUTTON_STYLE_DANGER:
            colorTop = BUTTON_DANGER_COLOR_TOP;
            colorBottom = BUTTON_DANGER_COLOR_BOTTOM;
            colorPressed = BUTTON_DANGER_COLOR_PRESSED;
            colorHighlight = BUTTON_DANGER_HIGHLIGHT;
            colorBorder = BUTTON_DANGER_BORDER;
            break;
        case BUTTON_STYLE_SECONDARY:
            colorTop = BUTTON2_COLOR_TOP;
            colorBottom = BUTTON2_COLOR_BOTTOM;
            colorPressed = BUTTON2_COLOR_PRESSED;
            colorHighlight = BUTTON2_HIGHLIGHT;
            colorBorder = BUTTON2_BORDER;
            break;
        default:
            colorTop = BUTTON_COLOR_TOP;
            colorBottom = BUTTON_COLOR_BOTTOM;
            colorPressed = BUTTON_COLOR_PRESSED;
            colorHighlight = BUTTON_HIGHLIGHT;
            colorBorder = BUTTON_BORDER;
            break;
    }
    
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
        draw_button(BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT, "Save and Connect", saveButtonPressed, BUTTON_STYLE_PRIMARY);
        draw_button(BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, "Cancel", cancelButtonPressed, BUTTON_STYLE_SECONDARY);
    } else {
        draw_button(BUTTON_X, SAVE_BUTTON_Y_SINGLE, BUTTON_WIDTH, BUTTON_HEIGHT, "Save and Connect", saveButtonPressed, BUTTON_STYLE_PRIMARY);
    }
}

static void draw_rom_detail_screen(void) {
    // Background
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
    
    // Toolbar at top with gear, queue, and bug icons
    draw_toolbar();
    
    // Download button (top)
    const char *downloadLabel = romExists ? "Download Again" : "Download";
    draw_button(BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT, downloadLabel, downloadButtonPressed, BUTTON_STYLE_PRIMARY);
    
    // Queue button (bottom)
    const char *queueLabel = romQueued ? "Remove from Queue" : "Add to Queue";
    ButtonStyle queueStyle = romQueued ? BUTTON_STYLE_DANGER : BUTTON_STYLE_SECONDARY;
    draw_button(BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, queueLabel, queueButtonPressed, queueStyle);
}

static void draw_downloading_screen(void) {
    // Background
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
    
    // Cancel button centered
    draw_button(BUTTON_X, SAVE_BUTTON_Y_SINGLE, BUTTON_WIDTH, BUTTON_HEIGHT, "Cancel Download", false, BUTTON_STYLE_DANGER);
    

}

static void draw_queue_screen(void) {
    // Background
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
    
    // Toolbar at top
    draw_toolbar();
    
    if (queueItemCount > 0) {
        // Start Downloads button (top)
        draw_button(BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT, "Start Downloads", startDownloadsPressed, BUTTON_STYLE_PRIMARY);
        // Clear Queue button (bottom)
        draw_button(BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, "Clear Queue", clearQueuePressed, BUTTON_STYLE_DANGER);
    }
}

static void draw_queue_confirm_screen(void) {
    // Background
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
    
    // Toolbar at top
    draw_toolbar();
    
    // "Are you sure?" text
    const char *prompt = "Are you sure?";
    float promptWidth = ui_get_text_width(prompt);
    ui_draw_text((SCREEN_BOTTOM_WIDTH - promptWidth) / 2, SAVE_BUTTON_Y_DUAL - UI_LINE_HEIGHT - UI_PADDING,
                 prompt, UI_COLOR_TEXT);
    
    // Clear Queue button (top, danger)
    draw_button(BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT, "Clear Queue", confirmClearPressed, BUTTON_STYLE_DANGER);
    // Cancel button (bottom)
    draw_button(BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, "Cancel", cancelClearPressed, BUTTON_STYLE_SECONDARY);
}

void bottom_draw(void) {
    C2D_SceneBegin(bottomTarget);
    C2D_TargetClear(bottomTarget, UI_COLOR_BG);
    
    if (showDebugModal) {
        draw_debug_modal();
    } else if (currentMode == BOTTOM_MODE_SETTINGS) {
        draw_settings_screen();
    } else if (currentMode == BOTTOM_MODE_ROMS || currentMode == BOTTOM_MODE_ROM_DETAIL) {
        draw_rom_detail_screen();
    } else if (currentMode == BOTTOM_MODE_DOWNLOADING) {
        draw_downloading_screen();
    } else if (currentMode == BOTTOM_MODE_QUEUE) {
        draw_queue_screen();
    } else if (currentMode == BOTTOM_MODE_QUEUE_CONFIRM) {
        draw_queue_confirm_screen();
    } else if (currentMode == BOTTOM_MODE_SEARCH_FORM) {
        // Background
        ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
        draw_toolbar();
        search_form_draw();
    } else if (currentMode == BOTTOM_MODE_SEARCH_RESULTS) {
        draw_rom_detail_screen();
    } else {
        draw_toolbar();
    }
}

void bottom_log_subscriber(LogLevel level, const char *message) {
    // Format with level prefix
    const char *levelName = log_level_name(level);
    char formatted[LOG_LINE_LENGTH];
    snprintf(formatted, sizeof(formatted), "[%s] %s", levelName, message);
    formatted[LOG_LINE_LENGTH - 1] = '\0';
    
    // Store in circular buffer
    snprintf(logBuffer[logHead], LOG_LINE_LENGTH, "%s", formatted);
    logBuffer[logHead][LOG_LINE_LENGTH - 1] = '\0';
    
    // Advance head
    logHead = (logHead + 1) % LOG_MAX_LINES;
    if (logCount < LOG_MAX_LINES) {
        logCount++;
    }
}

bool bottom_check_cancel(void) {
    hidScanInput();
    u32 kDown = hidKeysDown();
    
    if (kDown & KEY_TOUCH) {
        touchPosition touch;
        hidTouchRead(&touch);
        if (touch_in_rect(touch.px, touch.py, BUTTON_X, SAVE_BUTTON_Y_SINGLE, BUTTON_WIDTH, BUTTON_HEIGHT)) {
            return true;
        }
    }
    
    return false;
}
