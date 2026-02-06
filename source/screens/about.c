/*
 * About screen - App info and sponsor link
 */

#include "about.h"
#include "../ui.h"

AboutResult about_update(u32 kDown) {
    if (kDown & KEY_B) return ABOUT_BACK;
    return ABOUT_NONE;
}

void about_draw(void) {
    const char *appName = "Rommlet";
    float nameW = ui_get_text_width_scaled(appName, 1.0f);
    ui_draw_text_scaled((SCREEN_TOP_WIDTH - nameW) / 2, 40, appName, UI_COLOR_TEXT, 1.0f);

    const char *tagline = "A RomM client for Nintendo 3DS";
    float tagW = ui_get_text_width(tagline);
    ui_draw_text((SCREEN_TOP_WIDTH - tagW) / 2, 70, tagline, UI_COLOR_TEXT_DIM);

    const char *version = "v" APP_VERSION;
    float verW = ui_get_text_width_scaled(version, 0.45f);
    ui_draw_text_scaled((SCREEN_TOP_WIDTH - verW) / 2, 90, version, UI_COLOR_TEXT_DIM, 0.45f);

    float contentWidth = SCREEN_TOP_WIDTH - (UI_PADDING * 2);
    ui_draw_wrapped_text(UI_PADDING, 125, contentWidth,
                         "Rommlet is a free, open source application. "
                         "To say thanks, scan the QR code below to sponsor the project.",
                         UI_COLOR_TEXT, 4, 0);
}
