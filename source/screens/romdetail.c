/*
 * ROM detail screen - Display ROM information
 */

#include "romdetail.h"
#include "../ui.h"
#include <stdio.h>
#include <string.h>

static RomDetail *currentDetail = NULL;
static int scrollOffset = 0;

void romdetail_init(void) {
    currentDetail = NULL;
    scrollOffset = 0;
}

void romdetail_set_data(RomDetail *detail) {
    currentDetail = detail;
    scrollOffset = 0;
}

RomDetailResult romdetail_update(u32 kDown) {
    // Back to ROM list
    if (kDown & KEY_B) {
        return ROMDETAIL_BACK;
    }
    
    // Scroll description if needed
    if (kDown & KEY_DOWN) {
        scrollOffset++;
    }
    if (kDown & KEY_UP) {
        if (scrollOffset > 0) scrollOffset--;
    }
    
    return ROMDETAIL_NONE;
}

// Helper to draw wrapped text and return lines used
static int draw_wrapped_text(float x, float y, float maxWidth, const char *text, u32 color, int maxLines, int skipLines) {
    if (!text || !text[0]) return 0;
    
    char line[128];
    int lineStart = 0;
    int lineCount = 0;
    int drawnLines = 0;
    int len = strlen(text);
    
    for (int i = 0; i <= len; i++) {
        bool endOfWord = (text[i] == ' ' || text[i] == '\0' || text[i] == '\n');
        
        if (endOfWord || i - lineStart >= (int)sizeof(line) - 1) {
            // Check if this segment fits
            int segLen = i - lineStart;
            if (segLen > 0) {
                strncpy(line, &text[lineStart], segLen);
                line[segLen] = '\0';
                
                float textWidth = ui_get_text_width(line);
                if (textWidth > maxWidth && segLen > 10) {
                    // Line too long, find a break point
                    int breakPoint = segLen;
                    for (int j = segLen - 1; j > 0; j--) {
                        if (text[lineStart + j] == ' ') {
                            breakPoint = j;
                            break;
                        }
                    }
                    strncpy(line, &text[lineStart], breakPoint);
                    line[breakPoint] = '\0';
                    i = lineStart + breakPoint;
                }
                
                if (lineCount >= skipLines && drawnLines < maxLines) {
                    ui_draw_text(x, y + drawnLines * UI_LINE_HEIGHT, line, color);
                    drawnLines++;
                }
                lineCount++;
                lineStart = i + 1;
            }
        }
        
        if (text[i] == '\n') {
            lineStart = i + 1;
        }
        
        if (drawnLines >= maxLines) break;
    }
    
    return drawnLines;
}

void romdetail_draw(void) {
    if (!currentDetail) {
        ui_draw_header("ROM Details");
        ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT / 2, "No ROM selected.", UI_COLOR_TEXT_DIM);
        return;
    }
    
    ui_draw_header("ROM Details");
    
    float y = UI_HEADER_HEIGHT + UI_PADDING;
    float contentWidth = SCREEN_TOP_WIDTH - (UI_PADDING * 2);
    
    // Title (prominent)
    ui_draw_text(UI_PADDING, y, currentDetail->name, UI_COLOR_TEXT);
    y += UI_LINE_HEIGHT + UI_PADDING;
    
    // Platform
    if (currentDetail->platformName[0]) {
        char platformText[160];
        snprintf(platformText, sizeof(platformText), "Platform: %s", currentDetail->platformName);
        ui_draw_text(UI_PADDING, y, platformText, UI_COLOR_TEXT_DIM);
        y += UI_LINE_HEIGHT;
    }
    
    // Release date
    if (currentDetail->firstReleaseDate[0]) {
        char dateText[64];
        snprintf(dateText, sizeof(dateText), "Released: %s", currentDetail->firstReleaseDate);
        ui_draw_text(UI_PADDING, y, dateText, UI_COLOR_TEXT_DIM);
        y += UI_LINE_HEIGHT;
    }
    
    // MD5 hash
    if (currentDetail->md5Hash[0]) {
        char hashText[96];
        snprintf(hashText, sizeof(hashText), "MD5: %.32s...", currentDetail->md5Hash);
        ui_draw_text(UI_PADDING, y, hashText, UI_COLOR_TEXT_DIM);
        y += UI_LINE_HEIGHT;
    }
    
    y += UI_PADDING;
    
    // Description/Summary with wrapping
    if (currentDetail->summary[0]) {
        ui_draw_text(UI_PADDING, y, "Description:", UI_COLOR_TEXT_DIM);
        y += UI_LINE_HEIGHT;
        
        int maxDescLines = (SCREEN_TOP_HEIGHT - y - UI_LINE_HEIGHT - UI_PADDING * 2) / UI_LINE_HEIGHT;
        draw_wrapped_text(UI_PADDING, y, contentWidth, currentDetail->summary, 
                         UI_COLOR_TEXT, maxDescLines, scrollOffset);
    }
    
    // Help text
    ui_draw_text(UI_PADDING, SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT - UI_PADDING,
                 "D-Pad: Scroll | B: Back", UI_COLOR_TEXT_DIM);
}
