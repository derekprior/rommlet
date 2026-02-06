/*
 * Debug log viewer - Scrollable modal overlay for log messages
 */

#include "debuglog.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <3ds.h>

#define LOG_MAX_LINES 100
#define LOG_LINE_LENGTH 64

// Close button position (top-right)
#define CLOSE_X (SCREEN_BOTTOM_WIDTH - 20 - 4)
#define CLOSE_Y 4

static bool visible = false;

// Circular log buffer
static char logBuffer[LOG_MAX_LINES][LOG_LINE_LENGTH];
static int logHead = 0;
static int logCount = 0;

// Scroll state
static int scrollY = 0;
static int scrollX = 0;
static int lastTouchY = -1;

// Layout (computed during draw)
static float logAreaTop = 0;
static float logAreaHeight = 0;
static int visibleLines = 0;

void debuglog_init(void) {
    visible = false;
    scrollY = 0;
    scrollX = 0;
    lastTouchY = -1;
    logHead = 0;
    logCount = 0;
    memset(logBuffer, 0, sizeof(logBuffer));
}

bool debuglog_is_visible(void) {
    return visible;
}

void debuglog_show(void) {
    visible = true;
    scrollY = 0;
    scrollX = 0;
    lastTouchY = -1;
}

void debuglog_update(void) {
    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();

    // Close on X button tap
    if (kDown & KEY_TOUCH) {
        touchPosition touch;
        hidTouchRead(&touch);
        if (ui_touch_in_rect(touch.px, touch.py, CLOSE_X, CLOSE_Y, 20, 20)) {
            visible = false;
            lastTouchY = -1;
            return;
        }
        lastTouchY = touch.py;
    }

    // Touch drag scrolling
    if (kHeld & KEY_TOUCH) {
        touchPosition touch;
        hidTouchRead(&touch);
        if (touch.py >= logAreaTop && touch.py < logAreaTop + logAreaHeight) {
            if (lastTouchY >= 0) {
                int deltaY = lastTouchY - touch.py;
                if (deltaY != 0 && logCount > visibleLines) {
                    int maxScroll = logCount - visibleLines;
                    int scrollAmount = deltaY / 10;
                    if (scrollAmount != 0) {
                        scrollY += scrollAmount;
                        if (scrollY < 0) scrollY = 0;
                        if (scrollY > maxScroll) scrollY = maxScroll;
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

    // Log level cycling
    if (kDown & KEY_ZR) {
        LogLevel current = log_get_level();
        if (current == LOG_INFO) log_set_level(LOG_DEBUG);
        else if (current == LOG_DEBUG) log_set_level(LOG_TRACE);
        else log_set_level(LOG_INFO);
        log_info("Log level: %s", log_level_name(log_get_level()));
        return;
    }
    if (kDown & KEY_ZL) {
        LogLevel current = log_get_level();
        if (current == LOG_INFO) log_set_level(LOG_TRACE);
        else if (current == LOG_TRACE) log_set_level(LOG_DEBUG);
        else log_set_level(LOG_INFO);
        log_info("Log level: %s", log_level_name(log_get_level()));
        return;
    }

    // C-stick scrolling
    circlePosition cstick;
    hidCstickRead(&cstick);
    if (cstick.dy > 40 && scrollY > 0) scrollY--;
    if (cstick.dy < -40) {
        int maxScroll = logCount > visibleLines ? logCount - visibleLines : 0;
        if (scrollY < maxScroll) scrollY++;
    }
    if (cstick.dx > 40) {
        scrollX += 4;
        int maxScrollX = LOG_LINE_LENGTH * 8;
        if (scrollX > maxScrollX) scrollX = maxScrollX;
    }
    if (cstick.dx < -40) {
        scrollX -= 4;
        if (scrollX < 0) scrollX = 0;
    }
}

void debuglog_draw(void) {
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
    ui_draw_header_bottom("Debug Log");

    // Close button
    ui_draw_text(CLOSE_X + 4, CLOSE_Y + 2, "X", UI_COLOR_TEXT);

    // Level hint
    char levelHint[64];
    snprintf(levelHint, sizeof(levelHint), "ZL/ZR: Level (%s)", log_level_name(log_get_level()));
    ui_draw_text(UI_PADDING, UI_HEADER_HEIGHT + UI_PADDING, levelHint, UI_COLOR_TEXT_DIM);

    // Log content area
    logAreaTop = UI_HEADER_HEIGHT + UI_PADDING + UI_LINE_HEIGHT + UI_PADDING;
    logAreaHeight = SCREEN_BOTTOM_HEIGHT - logAreaTop - UI_PADDING;
    visibleLines = (int)(logAreaHeight / UI_LINE_HEIGHT);

    float y = logAreaTop;
    for (int i = 0; i < visibleLines && i + scrollY < logCount; i++) {
        int lineIndex = (logHead - logCount + i + scrollY + LOG_MAX_LINES) % LOG_MAX_LINES;
        ui_draw_text(UI_PADDING - scrollX, y, logBuffer[lineIndex], UI_COLOR_TEXT);
        y += UI_LINE_HEIGHT;
    }
}

void debuglog_subscriber(LogLevel level, const char *message) {
    const char *levelName = log_level_name(level);
    char formatted[LOG_LINE_LENGTH];
    snprintf(formatted, sizeof(formatted), "[%s] %s", levelName, message);
    snprintf(logBuffer[logHead], LOG_LINE_LENGTH, "%s", formatted);
    logHead = (logHead + 1) % LOG_MAX_LINES;
    if (logCount < LOG_MAX_LINES) logCount++;
}
