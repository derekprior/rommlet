/*
 * UI module - Graphics helpers using citro2d
 */

#ifndef UI_H
#define UI_H

#include <3ds.h>
#include <citro2d.h>

// Screen dimensions
#define SCREEN_TOP_WIDTH 400
#define SCREEN_TOP_HEIGHT 240
#define SCREEN_BOTTOM_WIDTH 320
#define SCREEN_BOTTOM_HEIGHT 240

// Colors (RGBA8 format)
#define UI_COLOR_BG         C2D_Color32(0x1a, 0x1a, 0x2e, 0xFF)
#define UI_COLOR_TEXT       C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
#define UI_COLOR_TEXT_DIM   C2D_Color32(0x88, 0x88, 0x88, 0xFF)
#define UI_COLOR_SELECTED   C2D_Color32(0x4a, 0x4a, 0xe0, 0xFF)
#define UI_COLOR_ACCENT     C2D_Color32(0x7c, 0x3a, 0xed, 0xFF)
#define UI_COLOR_HEADER     C2D_Color32(0x2d, 0x2d, 0x44, 0xFF)
#define UI_COLOR_SCROLLBAR_TRACK C2D_Color32(0x3a, 0x3a, 0x50, 0xFF)
#define UI_COLOR_SCROLLBAR_THUMB C2D_Color32(0x6a, 0x6a, 0x90, 0xFF)

// Layout constants
#define UI_PADDING 8
#define UI_LINE_HEIGHT 20
#define UI_HEADER_HEIGHT 30
#define UI_VISIBLE_ITEMS 8

// API/data constants
#define ROM_PAGE_SIZE 50

// Initialize UI module (load font, etc)
void ui_init(void);

// Cleanup UI module
void ui_exit(void);

// Draw text at position
void ui_draw_text(float x, float y, const char *text, u32 color);

// Draw text with scale
void ui_draw_text_scaled(float x, float y, const char *text, u32 color, float scale);

// Draw a filled rectangle
void ui_draw_rect(float x, float y, float w, float h, u32 color);

// Draw a list item (text with optional selection highlight)
void ui_draw_list_item(float x, float y, float w, const char *text, bool selected);

// Draw a header bar
void ui_draw_header(const char *title);

// Draw a header bar for bottom screen (narrower)
void ui_draw_header_bottom(const char *title);

// Get text width
float ui_get_text_width(const char *text);

// Draw a centered loading message on top screen
void ui_draw_loading(const char *message);

// Draw download progress on top screen (progress 0.0 to 1.0, negative if unknown)
void ui_draw_download_progress(float progress, const char *sizeText);

// Show software keyboard and get input
// Returns true if user confirmed, false if cancelled
bool ui_show_keyboard(const char *hint, char *buffer, size_t bufferSize, bool password);

#endif // UI_H
