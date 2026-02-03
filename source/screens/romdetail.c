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
    
    char line[256];
    char word[128];
    int lineLen = 0;
    int wordLen = 0;
    int lineCount = 0;
    int drawnLines = 0;
    int len = strlen(text);
    
    line[0] = '\0';
    
    for (int i = 0; i <= len; i++) {
        char c = text[i];
        bool isSpace = (c == ' ' || c == '\0' || c == '\n');
        
        if (isSpace) {
            if (wordLen > 0) {
                word[wordLen] = '\0';
                
                // Check if word fits on current line
                char testLine[256];
                if (lineLen > 0) {
                    snprintf(testLine, sizeof(testLine), "%s %s", line, word);
                } else {
                    snprintf(testLine, sizeof(testLine), "%s", word);
                }
                
                float testWidth = ui_get_text_width(testLine);
                
                if (testWidth <= maxWidth || lineLen == 0) {
                    // Word fits, add to line
                    if (lineLen > 0) {
                        strcat(line, " ");
                    }
                    strcat(line, word);
                    lineLen = strlen(line);
                } else {
                    // Word doesn't fit, output current line and start new one
                    if (lineCount >= skipLines && drawnLines < maxLines) {
                        ui_draw_text(x, y + drawnLines * UI_LINE_HEIGHT, line, color);
                        drawnLines++;
                    }
                    lineCount++;
                    
                    strcpy(line, word);
                    lineLen = strlen(line);
                }
                
                wordLen = 0;
            }
            
            // Handle newline
            if (c == '\n' && lineLen > 0) {
                if (lineCount >= skipLines && drawnLines < maxLines) {
                    ui_draw_text(x, y + drawnLines * UI_LINE_HEIGHT, line, color);
                    drawnLines++;
                }
                lineCount++;
                line[0] = '\0';
                lineLen = 0;
            }
        } else {
            if (wordLen < (int)sizeof(word) - 1) {
                word[wordLen++] = c;
            }
        }
        
        if (drawnLines >= maxLines && lineCount >= skipLines) break;
    }
    
    // Output remaining line
    if (lineLen > 0 && drawnLines < maxLines && lineCount >= skipLines) {
        ui_draw_text(x, y + drawnLines * UI_LINE_HEIGHT, line, color);
        drawnLines++;
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
    y += UI_LINE_HEIGHT;
    
    // Platform (no label)
    if (currentDetail->platformName[0]) {
        ui_draw_text(UI_PADDING, y, currentDetail->platformName, UI_COLOR_TEXT_DIM);
        y += UI_LINE_HEIGHT;
    }
    
    // Release date
    if (currentDetail->firstReleaseDate[0]) {
        char dateText[64];
        snprintf(dateText, sizeof(dateText), "Released: %s", currentDetail->firstReleaseDate);
        ui_draw_text(UI_PADDING, y, dateText, UI_COLOR_TEXT_DIM);
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
