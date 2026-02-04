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
