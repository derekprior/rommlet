/*
 * UI module - Graphics helpers using citro2d
 */

#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

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

void ui_draw_progress(float progress, const char *label, const char *sizeText, const char *name,
                      const char *queueText) {
    ui_draw_rect(0, 0, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, C2D_Color32(0x1a, 0x1a, 0x2e, 0xE0));

    float centerY = SCREEN_TOP_HEIGHT / 2;

    // ROM name above progress bar
    if (name) {
        float nameWidth = ui_get_text_width(name);
        ui_draw_text((SCREEN_TOP_WIDTH - nameWidth) / 2, centerY - 2 * UI_LINE_HEIGHT - UI_PADDING, name,
                     UI_COLOR_TEXT);
    }

    // Action label
    float labelWidth = ui_get_text_width(label);
    ui_draw_text((SCREEN_TOP_WIDTH - labelWidth) / 2, centerY - UI_LINE_HEIGHT - UI_PADDING, label, UI_COLOR_TEXT_DIM);

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

    // Queue context below size text
    if (queueText) {
        float queueWidth = ui_get_text_width(queueText);
        ui_draw_text((SCREEN_TOP_WIDTH - queueWidth) / 2, barY + barHeight + UI_PADDING + UI_LINE_HEIGHT, queueText,
                     UI_COLOR_TEXT_DIM);
    }
}

float ui_get_text_width(const char *text) {
    return ui_get_text_width_scaled(text, 0.5f);
}

float ui_get_text_width_scaled(const char *text, float scale) {
    C2D_TextBufClear(textBuf);

    C2D_Text c2dText;
    if (fontLoaded) {
        C2D_TextFontParse(&c2dText, font, textBuf, text);
    } else {
        C2D_TextParse(&c2dText, textBuf, text);
    }

    float width, height;
    C2D_TextGetDimensions(&c2dText, scale, scale, &width, &height);
    return width;
}

bool ui_show_keyboard(const char *hint, char *buffer, size_t bufferSize, bool password) {
    (void)password; // No longer used - show all input as plaintext
    static SwkbdState swkbd;
    static char tempBuf[256];

    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, bufferSize - 1);
    swkbdSetHintText(&swkbd, hint);
    swkbdSetInitialText(&swkbd, buffer);

    swkbdSetFeatures(&swkbd, SWKBD_DEFAULT_QWERTY);
    swkbdSetValidation(&swkbd, SWKBD_ANYTHING, 0, 0);

    SwkbdButton button = swkbdInputText(&swkbd, tempBuf, sizeof(tempBuf));

    if (button == SWKBD_BUTTON_CONFIRM) {
        snprintf(buffer, bufferSize, "%s", tempBuf);
        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// Touch utility
// ---------------------------------------------------------------------------

bool ui_touch_in_rect(int tx, int ty, int x, int y, int w, int h) {
    return tx >= x && tx < x + w && ty >= y && ty < y + h;
}

// ---------------------------------------------------------------------------
// Button widget
// ---------------------------------------------------------------------------

// Primary button colors (green)
#define BTN_PRI_TOP C2D_Color32(0x5a, 0xa0, 0x5a, 0xFF)
#define BTN_PRI_BOT C2D_Color32(0x3a, 0x80, 0x3a, 0xFF)
#define BTN_PRI_PRESS C2D_Color32(0x2a, 0x60, 0x2a, 0xFF)
#define BTN_PRI_HI C2D_Color32(0x7a, 0xc0, 0x7a, 0xFF)
#define BTN_PRI_BORDER C2D_Color32(0x2a, 0x50, 0x2a, 0xFF)

// Secondary button colors (gray)
#define BTN_SEC_TOP C2D_Color32(0x6a, 0x6a, 0x70, 0xFF)
#define BTN_SEC_BOT C2D_Color32(0x50, 0x50, 0x56, 0xFF)
#define BTN_SEC_PRESS C2D_Color32(0x40, 0x40, 0x46, 0xFF)
#define BTN_SEC_HI C2D_Color32(0x8a, 0x8a, 0x90, 0xFF)
#define BTN_SEC_BORDER C2D_Color32(0x3a, 0x3a, 0x40, 0xFF)

// Danger button colors (red)
#define BTN_DGR_TOP C2D_Color32(0xc0, 0x40, 0x40, 0xFF)
#define BTN_DGR_BOT C2D_Color32(0xa0, 0x30, 0x30, 0xFF)
#define BTN_DGR_PRESS C2D_Color32(0x80, 0x20, 0x20, 0xFF)
#define BTN_DGR_HI C2D_Color32(0xe0, 0x60, 0x60, 0xFF)
#define BTN_DGR_BORDER C2D_Color32(0x60, 0x20, 0x20, 0xFF)

#define BTN_SHADOW C2D_Color32(0x1a, 0x1a, 0x2e, 0x80)

void ui_draw_button(float x, float y, float w, float h, const char *text, bool pressed, UIButtonStyle style) {
    if (!pressed) {
        ui_draw_rect(x + 3, y + 3, w, h, BTN_SHADOW);
    }

    float bx = pressed ? x + 1 : x;
    float by = pressed ? y + 1 : y;

    u32 colorTop, colorBottom, colorPressed, colorHighlight, colorBorder;
    switch (style) {
    case UI_BUTTON_DANGER:
        colorTop = BTN_DGR_TOP;
        colorBottom = BTN_DGR_BOT;
        colorPressed = BTN_DGR_PRESS;
        colorHighlight = BTN_DGR_HI;
        colorBorder = BTN_DGR_BORDER;
        break;
    case UI_BUTTON_SECONDARY:
        colorTop = BTN_SEC_TOP;
        colorBottom = BTN_SEC_BOT;
        colorPressed = BTN_SEC_PRESS;
        colorHighlight = BTN_SEC_HI;
        colorBorder = BTN_SEC_BORDER;
        break;
    default:
        colorTop = BTN_PRI_TOP;
        colorBottom = BTN_PRI_BOT;
        colorPressed = BTN_PRI_PRESS;
        colorHighlight = BTN_PRI_HI;
        colorBorder = BTN_PRI_BORDER;
        break;
    }

    ui_draw_rect(bx - 2, by - 2, w + 4, h + 4, colorBorder);

    if (pressed) {
        ui_draw_rect(bx, by, w, h, colorPressed);
    } else {
        ui_draw_rect(bx, by, w, h / 2, colorTop);
        ui_draw_rect(bx, by + h / 2, w, h / 2, colorBottom);
        ui_draw_rect(bx, by, w, 2, colorHighlight);
    }

    float textWidth = ui_get_text_width(text);
    float textX = bx + (w - textWidth) / 2;
    float textY = by + (h - 16) / 2;
    ui_draw_text(textX, textY, text, UI_COLOR_TEXT);
}

// ---------------------------------------------------------------------------
// Dock button (full-width, rounded top corners, subtle style)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Icons (designed for 20px, scale to any size)
// ---------------------------------------------------------------------------

void ui_draw_icon_bug(float x, float y, float size, u32 color) {
    float cx = x + size / 2;
    float cy = y + size / 2;
    float scale = size / 20.0f;

    // Body
    float bodyW = 6 * scale;
    float bodyH = 8 * scale;
    C2D_DrawEllipseSolid(cx - bodyW / 2, cy - bodyH / 2 + 2 * scale, 0, bodyW, bodyH, color);

    // Head
    C2D_DrawCircleSolid(cx, cy - 4 * scale, 0, 3 * scale, color);

    // Antennae
    C2D_DrawRectSolid(cx - 3 * scale, cy - 7 * scale, 0, 1 * scale, 3 * scale, color);
    C2D_DrawRectSolid(cx + 2 * scale, cy - 7 * scale, 0, 1 * scale, 3 * scale, color);

    // Left legs
    C2D_DrawRectSolid(cx - 6 * scale, cy - 1 * scale, 0, 4 * scale, 1 * scale, color);
    C2D_DrawRectSolid(cx - 6 * scale, cy + 2 * scale, 0, 4 * scale, 1 * scale, color);
    C2D_DrawRectSolid(cx - 5 * scale, cy + 5 * scale, 0, 3 * scale, 1 * scale, color);
    // Right legs
    C2D_DrawRectSolid(cx + 2 * scale, cy - 1 * scale, 0, 4 * scale, 1 * scale, color);
    C2D_DrawRectSolid(cx + 2 * scale, cy + 2 * scale, 0, 4 * scale, 1 * scale, color);
    C2D_DrawRectSolid(cx + 2 * scale, cy + 5 * scale, 0, 3 * scale, 1 * scale, color);
}

void ui_draw_icon_gear(float x, float y, float size, u32 color) {
    float cx = x + size / 2;
    float cy = y + size / 2;
    float scale = size / 20.0f;

    float innerR = 3 * scale;
    float outerR = 6 * scale;

    C2D_DrawCircleSolid(cx, cy, 0, outerR, color);

    float toothLen = 3 * scale;
    float toothWidth = 2.5 * scale;
    for (int i = 0; i < 8; i++) {
        float angle = i * 3.14159f / 4.0f;
        float toothCx = cx + (outerR + toothLen / 2 - 2) * cosf(angle);
        float toothCy = cy + (outerR + toothLen / 2 - 2) * sinf(angle);
        C2D_DrawRectSolid(toothCx - toothWidth / 2, toothCy - toothWidth / 2, 0, toothWidth, toothWidth, color);
    }

    C2D_DrawCircleSolid(cx, cy, 0, innerR, UI_COLOR_HEADER);
}

void ui_draw_icon_queue(float x, float y, float size, u32 color) {
    float scale = size / 20.0f;
    float lx = x + 3 * scale;
    float ly = y + 4 * scale;
    float lineW = 14 * scale;
    float lineH = 2 * scale;
    float gap = 4 * scale;

    C2D_DrawRectSolid(lx, ly, 0, lineW, lineH, color);
    C2D_DrawRectSolid(lx, ly + gap, 0, lineW, lineH, color);
    C2D_DrawRectSolid(lx, ly + gap * 2, 0, lineW, lineH, color);

    float ax = x + 14 * scale;
    float ay = ly + gap * 2 + lineH + 1 * scale;
    C2D_DrawRectSolid(ax, ay, 0, 3 * scale, 2 * scale, color);
}

void ui_draw_icon_search(float x, float y, float size, u32 color) {
    float cx = x + size / 2;
    float cy = y + size / 2;
    float scale = size / 20.0f;

    float lensR = 5 * scale;
    float lensCx = cx - 2 * scale;
    float lensCy = cy - 2 * scale;
    C2D_DrawCircleSolid(lensCx, lensCy, 0, lensR, color);
    C2D_DrawCircleSolid(lensCx, lensCy, 0, lensR - 2 * scale, UI_COLOR_HEADER);

    float hw = 2.5f * scale;
    for (int i = 0; i < 4; i++) {
        float offset = (3 + i * 1.5f) * scale;
        C2D_DrawRectSolid(lensCx + offset - hw / 2, lensCy + offset - hw / 2, 0, hw, hw, color);
    }
}

void ui_draw_icon_home(float x, float y, float size, u32 color) {
    float cx = x + size / 2;
    float scale = size / 20.0f;

    float roofTop = y + 2 * scale;
    float roofMid = y + 9 * scale;
    float halfW = 8 * scale;

    for (int i = 0; i < 4; i++) {
        float fy = roofTop + i * 2 * scale;
        float fw = (i + 1) * 2 * scale;
        C2D_DrawRectSolid(cx - fw, fy, 0, fw, 2 * scale, color);
    }
    for (int i = 0; i < 4; i++) {
        float fy = roofTop + i * 2 * scale;
        float fw = (i + 1) * 2 * scale;
        C2D_DrawRectSolid(cx, fy, 0, fw, 2 * scale, color);
    }

    C2D_DrawRectSolid(cx - halfW + 2 * scale, roofMid, 0, (halfW - 2 * scale) * 2, 8 * scale, color);
    C2D_DrawRectSolid(cx - 2 * scale, roofMid + 2 * scale, 0, 4 * scale, 6 * scale, UI_COLOR_HEADER);
}

void ui_draw_icon_info(float x, float y, float size, u32 color) {
    float cx = x + size / 2;
    float cy = y + size / 2;
    float scale = size / 20.0f;
    float radius = 9 * scale;

    // Filled circle
    C2D_DrawCircleSolid(cx, cy, 0, radius, color);

    // Negative-space "i" dot
    float dotR = 1.5f * scale;
    C2D_DrawCircleSolid(cx, cy - 4 * scale, 0, dotR, UI_COLOR_HEADER);

    // Negative-space "i" bar
    float barW = 3 * scale;
    float barH = 7 * scale;
    C2D_DrawRectSolid(cx - barW / 2, cy - 1 * scale, 0, barW, barH, UI_COLOR_HEADER);
}

// ---------------------------------------------------------------------------
// QR code (https://github.com/sponsors/derekprior)
// ---------------------------------------------------------------------------

#define QR_SIZE 29
static const uint8_t qr_data[QR_SIZE][QR_SIZE] = {
    {1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1},
    {1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1},
    {1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0},
    {1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1},
    {0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0},
    {0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1},
    {1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0},
    {1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0},
    {0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1},
    {1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1},
    {0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1},
    {1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0},
    {1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1},
    {1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0},
    {1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0},
};

void ui_draw_qr_code(float x, float y, float size) {
    float moduleSize = size / QR_SIZE;
    float padding = moduleSize * 2;
    C2D_DrawRectSolid(x - padding, y - padding, 0, size + padding * 2, size + padding * 2,
                      C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
    for (int row = 0; row < QR_SIZE; row++) {
        for (int col = 0; col < QR_SIZE; col++) {
            if (qr_data[row][col]) {
                C2D_DrawRectSolid(x + col * moduleSize, y + row * moduleSize, 0, moduleSize + 0.5f, moduleSize + 0.5f,
                                  C2D_Color32(0x00, 0x00, 0x00, 0xFF));
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Word-wrapped text
// ---------------------------------------------------------------------------

int ui_draw_wrapped_text(float x, float y, float maxWidth, const char *text, u32 color, int maxLines, int skipLines) {
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

                char testLine[256];
                if (lineLen > 0) {
                    snprintf(testLine, sizeof(testLine), "%s %s", line, word);
                } else {
                    snprintf(testLine, sizeof(testLine), "%s", word);
                }

                float testWidth = ui_get_text_width(testLine);

                if (testWidth <= maxWidth || lineLen == 0) {
                    if (lineLen > 0) {
                        snprintf(line + lineLen, sizeof(line) - lineLen, " %s", word);
                    } else {
                        snprintf(line, sizeof(line), "%s", word);
                    }
                    lineLen = strlen(line);
                } else {
                    if (lineCount >= skipLines && drawnLines < maxLines) {
                        ui_draw_text(x, y + drawnLines * UI_LINE_HEIGHT, line, color);
                        drawnLines++;
                    }
                    lineCount++;
                    snprintf(line, sizeof(line), "%s", word);
                    lineLen = strlen(line);
                }

                wordLen = 0;
            }

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

    if (lineLen > 0 && drawnLines < maxLines && lineCount >= skipLines) {
        ui_draw_text(x, y + drawnLines * UI_LINE_HEIGHT, line, color);
        drawnLines++;
    }

    return drawnLines;
}
