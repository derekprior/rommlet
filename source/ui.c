/*
 * UI module - Graphics helpers using citro2d
 */

#include "ui.h"
#include <stdio.h>
#include <string.h>

static C2D_TextBuf textBuf;
static C2D_Font font;
static bool fontLoaded = false;

void ui_init(void) {
    textBuf = C2D_TextBufNew(4096);
    
    // Try to load system font
    font = C2D_FontLoadSystem(CFG_REGION_USA);
    fontLoaded = (font != NULL);
}

void ui_exit(void) {
    if (fontLoaded && font) {
        C2D_FontFree(font);
    }
    C2D_TextBufDelete(textBuf);
}

void ui_draw_text(float x, float y, const char *text, u32 color) {
    ui_draw_text_scaled(x, y, text, color, 0.5f);
}

void ui_draw_text_scaled(float x, float y, const char *text, u32 color, float scale) {
    C2D_TextBufClear(textBuf);
    
    C2D_Text c2dText;
    if (fontLoaded) {
        C2D_TextFontParse(&c2dText, font, textBuf, text);
    } else {
        C2D_TextParse(&c2dText, textBuf, text);
    }
    C2D_TextOptimize(&c2dText);
    C2D_DrawText(&c2dText, C2D_WithColor, x, y, 0.5f, scale, scale, color);
}

void ui_draw_rect(float x, float y, float w, float h, u32 color) {
    C2D_DrawRectSolid(x, y, 0.0f, w, h, color);
}

void ui_draw_list_item(float x, float y, float w, const char *text, bool selected) {
    if (selected) {
        ui_draw_rect(x, y, w, UI_LINE_HEIGHT, UI_COLOR_SELECTED);
    }
    ui_draw_text(x + UI_PADDING, y + 2, text, UI_COLOR_TEXT);
}

void ui_draw_header(const char *title) {
    ui_draw_rect(0, 0, SCREEN_TOP_WIDTH, UI_HEADER_HEIGHT, UI_COLOR_HEADER);
    ui_draw_text_scaled(UI_PADDING, 5, title, UI_COLOR_TEXT, 0.7f);
}

void ui_draw_header_bottom(const char *title) {
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, UI_HEADER_HEIGHT, UI_COLOR_HEADER);
    ui_draw_text_scaled(UI_PADDING, 5, title, UI_COLOR_TEXT, 0.7f);
}

void ui_draw_loading(const char *message) {
    // Semi-transparent overlay
    ui_draw_rect(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, C2D_Color32(0x1a, 0x1a, 0x2e, 0xE0));
    
    // Center the message
    float textWidth = ui_get_text_width(message);
    float x = (SCREEN_TOP_WIDTH - textWidth) / 2;
    float y = (SCREEN_TOP_HEIGHT - UI_LINE_HEIGHT) / 2;
    
    ui_draw_text(x, y, message, UI_COLOR_TEXT);
}

void ui_draw_download_progress(float progress, const char *sizeText) {
    ui_draw_rect(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, C2D_Color32(0x1a, 0x1a, 0x2e, 0xE0));
    
    // "Downloading..." label
    const char *label = "Downloading...";
    float labelWidth = ui_get_text_width(label);
    float centerY = SCREEN_TOP_HEIGHT / 2;
    ui_draw_text((SCREEN_TOP_WIDTH - labelWidth) / 2, centerY - UI_LINE_HEIGHT - UI_PADDING, label, UI_COLOR_TEXT);
    
    // Progress bar
    float barWidth = 300;
    float barHeight = 16;
    float barX = (SCREEN_TOP_WIDTH - barWidth) / 2;
    float barY = centerY;
    
    // Track
    ui_draw_rect(barX, barY, barWidth, barHeight, UI_COLOR_SCROLLBAR_TRACK);
    
    // Fill
    if (progress >= 0) {
        float fillWidth = barWidth * progress;
        if (fillWidth > 0) {
            ui_draw_rect(barX, barY, fillWidth, barHeight, UI_COLOR_ACCENT);
        }
    }
    
    // Size text below bar
    if (sizeText) {
        float sizeWidth = ui_get_text_width(sizeText);
        ui_draw_text((SCREEN_TOP_WIDTH - sizeWidth) / 2, barY + barHeight + UI_PADDING, sizeText, UI_COLOR_TEXT_DIM);
    }
}

float ui_get_text_width(const char *text) {
    C2D_TextBufClear(textBuf);
    
    C2D_Text c2dText;
    if (fontLoaded) {
        C2D_TextFontParse(&c2dText, font, textBuf, text);
    } else {
        C2D_TextParse(&c2dText, textBuf, text);
    }
    
    float width, height;
    C2D_TextGetDimensions(&c2dText, 0.5f, 0.5f, &width, &height);
    return width;
}

bool ui_show_keyboard(const char *hint, char *buffer, size_t bufferSize, bool password) {
    (void)password;  // No longer used - show all input as plaintext
    SwkbdState swkbd;
    
    // Initialize keyboard - use NORMAL type for longer input support
    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, bufferSize - 1);
    swkbdSetHintText(&swkbd, hint);
    swkbdSetInitialText(&swkbd, buffer);
    
    // Allow empty input
    swkbdSetFeatures(&swkbd, SWKBD_DEFAULT_QWERTY);
    swkbdSetValidation(&swkbd, SWKBD_ANYTHING, 0, 0);
    
    char tempBuf[256];
    SwkbdButton button = swkbdInputText(&swkbd, tempBuf, sizeof(tempBuf));
    
    if (button == SWKBD_BUTTON_CONFIRM) {
        strncpy(buffer, tempBuf, bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        return true;
    }
    
    return false;
}
