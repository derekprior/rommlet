/*
 * Bottom screen - Toolbar, mode-specific button panels, and touch dispatch
 */

#include "bottom.h"
#include "search.h"
#include "../debuglog.h"
#include "../ui.h"
#include <stdio.h>
#include <string.h>

// Layout constants for bottom screen
#define TOOLBAR_HEIGHT 24
#define ICON_SIZE 20
#define ICON_PADDING 4

// Touch zones - right side: bug, gear, queue, search, info (right to left)
#define BUG_ICON_X (SCREEN_BOTTOM_WIDTH - ICON_SIZE - ICON_PADDING)
#define BUG_ICON_Y (ICON_PADDING)
#define GEAR_ICON_X (SCREEN_BOTTOM_WIDTH - (ICON_SIZE + ICON_PADDING) * 2)
#define GEAR_ICON_Y (ICON_PADDING)
#define QUEUE_ICON_X (SCREEN_BOTTOM_WIDTH - (ICON_SIZE + ICON_PADDING) * 3)
#define QUEUE_ICON_Y (ICON_PADDING)
#define SEARCH_ICON_X (SCREEN_BOTTOM_WIDTH - (ICON_SIZE + ICON_PADDING) * 4)
#define SEARCH_ICON_Y (ICON_PADDING)
#define INFO_ICON_X (SCREEN_BOTTOM_WIDTH - (ICON_SIZE + ICON_PADDING) * 5)
#define INFO_ICON_Y (ICON_PADDING)
// Left side: home icon
#define HOME_ICON_X (ICON_PADDING)
#define HOME_ICON_Y (ICON_PADDING)

static BottomMode currentMode = BOTTOM_MODE_DEFAULT;

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
static char folderName[256] = "";
static bool selectFolderPressed = false;
static bool createFolderPressed = false;
static bool showCancelButton = false;

// Render target for bottom screen
static C3D_RenderTarget *bottomTarget = NULL;

// Button dimensions
#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 50
#define BUTTON_SPACING 15
#define SAVE_BUTTON_Y_SINGLE ((SCREEN_BOTTOM_HEIGHT - BUTTON_HEIGHT) / 2)
#define SAVE_BUTTON_Y_DUAL ((SCREEN_BOTTOM_HEIGHT - BUTTON_HEIGHT * 2 - BUTTON_SPACING) / 2)
#define CANCEL_BUTTON_Y (SAVE_BUTTON_Y_DUAL + BUTTON_HEIGHT + BUTTON_SPACING)
#define BUTTON_X ((SCREEN_BOTTOM_WIDTH - BUTTON_WIDTH) / 2)

void bottom_init(void) {
    bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    currentMode = BOTTOM_MODE_DEFAULT;
    showCancelButton = false;
    saveButtonPressed = false;
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
    selectFolderPressed = false;
    createFolderPressed = false;
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

void bottom_set_folder_name(const char *name) {
    if (name) {
        snprintf(folderName, sizeof(folderName), "%s", name);
    } else {
        folderName[0] = '\0';
    }
}

BottomAction bottom_update(void) {
    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();
    u32 kUp = hidKeysUp();
    touchPosition touch;

    // Debug log modal consumes all input when visible
    if (debuglog_is_visible()) {
        debuglog_update();
        return BOTTOM_ACTION_NONE;
    }

    BottomAction action = BOTTOM_ACTION_NONE;

    // Handle settings mode buttons
    if (currentMode == BOTTOM_MODE_SETTINGS) {
        float saveButtonY = showCancelButton ? SAVE_BUTTON_Y_DUAL : SAVE_BUTTON_Y_SINGLE;

        if (kDown & KEY_TOUCH) {
            hidTouchRead(&touch);
            if (ui_touch_in_rect(touch.px, touch.py, BUTTON_X, saveButtonY, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                saveButtonPressed = true;
            }
            if (showCancelButton && ui_touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                cancelButtonPressed = true;
            }
        }
        if (kHeld & KEY_TOUCH) {
            hidTouchRead(&touch);
            saveButtonPressed = ui_touch_in_rect(touch.px, touch.py, BUTTON_X, saveButtonY, BUTTON_WIDTH, BUTTON_HEIGHT);
            if (showCancelButton) {
                cancelButtonPressed = ui_touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
            }
        }
        if (kUp & KEY_TOUCH) {
            if (saveButtonPressed) action = BOTTOM_ACTION_SAVE_SETTINGS;
            if (cancelButtonPressed) action = BOTTOM_ACTION_CANCEL_SETTINGS;
            saveButtonPressed = false;
            cancelButtonPressed = false;
        }
    }

    // Handle ROM actions mode buttons (download + queue)
    if (currentMode == BOTTOM_MODE_ROM_ACTIONS) {
        if (kDown & KEY_TOUCH) {
            hidTouchRead(&touch);
            if (ui_touch_in_rect(touch.px, touch.py, BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                downloadButtonPressed = true;
            }
            if (ui_touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                queueButtonPressed = true;
            }
        }
        if (kHeld & KEY_TOUCH) {
            hidTouchRead(&touch);
            downloadButtonPressed = ui_touch_in_rect(touch.px, touch.py, BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT);
            queueButtonPressed = ui_touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
        }
        if (kUp & KEY_TOUCH) {
            if (downloadButtonPressed) action = BOTTOM_ACTION_DOWNLOAD_ROM;
            if (queueButtonPressed) action = BOTTOM_ACTION_QUEUE_ROM;
            downloadButtonPressed = false;
            queueButtonPressed = false;
        }
    }

    // Handle queue mode buttons (start downloads + clear queue)
    if (currentMode == BOTTOM_MODE_QUEUE) {
        if (kDown & KEY_TOUCH) {
            hidTouchRead(&touch);
            if (ui_touch_in_rect(touch.px, touch.py, BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                startDownloadsPressed = true;
            }
            if (queueItemCount > 0 && ui_touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                clearQueuePressed = true;
            }
        }
        if (kHeld & KEY_TOUCH) {
            hidTouchRead(&touch);
            startDownloadsPressed = ui_touch_in_rect(touch.px, touch.py, BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT);
            if (queueItemCount > 0) {
                clearQueuePressed = ui_touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
            }
        }
        if (kUp & KEY_TOUCH) {
            if (startDownloadsPressed) action = BOTTOM_ACTION_START_DOWNLOADS;
            if (clearQueuePressed) action = BOTTOM_ACTION_CLEAR_QUEUE;
            startDownloadsPressed = false;
            clearQueuePressed = false;
        }
    }

    // Handle queue confirm clear dialog
    if (currentMode == BOTTOM_MODE_QUEUE_CONFIRM) {
        if (kDown & KEY_TOUCH) {
            hidTouchRead(&touch);
            if (ui_touch_in_rect(touch.px, touch.py, BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                confirmClearPressed = true;
            }
            if (ui_touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                cancelClearPressed = true;
            }
        }
        if (kHeld & KEY_TOUCH) {
            hidTouchRead(&touch);
            confirmClearPressed = ui_touch_in_rect(touch.px, touch.py, BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT);
            cancelClearPressed = ui_touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
        }
        if (kUp & KEY_TOUCH) {
            if (confirmClearPressed) action = BOTTOM_ACTION_CLEAR_QUEUE;
            if (cancelClearPressed) action = BOTTOM_ACTION_CANCEL_CLEAR;
            confirmClearPressed = false;
            cancelClearPressed = false;
        }
    }

    // Handle search form mode (search field tap + search button)
    if (currentMode == BOTTOM_MODE_SEARCH_FORM) {
        float fieldY = TOOLBAR_HEIGHT + UI_PADDING;
        float fieldH = 22;
        float btnY = SCREEN_BOTTOM_HEIGHT - 30 - UI_PADDING;
        float btnH = 30;
        float btnX = (SCREEN_BOTTOM_WIDTH - 200) / 2;
        float btnW = 200;

        if (kDown & KEY_TOUCH) {
            hidTouchRead(&touch);
            if (ui_touch_in_rect(touch.px, touch.py, UI_PADDING, fieldY, SCREEN_BOTTOM_WIDTH - UI_PADDING * 2, fieldH)) {
                action = BOTTOM_ACTION_SEARCH_FIELD;
            }
            if (ui_touch_in_rect(touch.px, touch.py, btnX, btnY, btnW, btnH)) {
                searchButtonPressed = true;
            }
        }
        if (kHeld & KEY_TOUCH) {
            hidTouchRead(&touch);
            searchButtonPressed = ui_touch_in_rect(touch.px, touch.py, btnX, btnY, btnW, btnH);
        }
        if (kUp & KEY_TOUCH) {
            if (searchButtonPressed) action = BOTTOM_ACTION_SEARCH_EXECUTE;
            searchButtonPressed = false;
        }
    }

    // Handle folder browser mode buttons (select + create)
    if (currentMode == BOTTOM_MODE_FOLDER_BROWSER) {
        if (kDown & KEY_TOUCH) {
            hidTouchRead(&touch);
            if (ui_touch_in_rect(touch.px, touch.py, BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                selectFolderPressed = true;
            }
            if (ui_touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT)) {
                createFolderPressed = true;
            }
        }
        if (kHeld & KEY_TOUCH) {
            hidTouchRead(&touch);
            selectFolderPressed = ui_touch_in_rect(touch.px, touch.py, BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT);
            createFolderPressed = ui_touch_in_rect(touch.px, touch.py, BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
        }
        if (kUp & KEY_TOUCH) {
            if (selectFolderPressed) action = BOTTOM_ACTION_SELECT_FOLDER;
            if (createFolderPressed) action = BOTTOM_ACTION_CREATE_FOLDER;
            selectFolderPressed = false;
            createFolderPressed = false;
        }
    }

    // Toolbar touch handling
    if (kDown & KEY_TOUCH) {
        hidTouchRead(&touch);

        // Bug icon opens debug log
        if (ui_touch_in_rect(touch.px, touch.py, BUG_ICON_X, BUG_ICON_Y, ICON_SIZE, ICON_SIZE)) {
            debuglog_show();
            return action;
        }
        // Settings icon
        if (currentMode != BOTTOM_MODE_SETTINGS &&
            ui_touch_in_rect(touch.px, touch.py, GEAR_ICON_X, GEAR_ICON_Y, ICON_SIZE, ICON_SIZE)) {
            return BOTTOM_ACTION_OPEN_SETTINGS;
        }
        // Queue icon
        if (currentMode != BOTTOM_MODE_QUEUE && currentMode != BOTTOM_MODE_QUEUE_CONFIRM &&
            ui_touch_in_rect(touch.px, touch.py, QUEUE_ICON_X, QUEUE_ICON_Y, ICON_SIZE, ICON_SIZE)) {
            return BOTTOM_ACTION_OPEN_QUEUE;
        }
        // Search icon
        if (currentMode != BOTTOM_MODE_SEARCH_FORM &&
            ui_touch_in_rect(touch.px, touch.py, SEARCH_ICON_X, SEARCH_ICON_Y, ICON_SIZE, ICON_SIZE)) {
            return BOTTOM_ACTION_OPEN_SEARCH;
        }
        // Info icon
        if (currentMode != BOTTOM_MODE_ABOUT &&
            ui_touch_in_rect(touch.px, touch.py, INFO_ICON_X, INFO_ICON_Y, ICON_SIZE, ICON_SIZE)) {
            return BOTTOM_ACTION_OPEN_ABOUT;
        }
        // Home icon
        if (ui_touch_in_rect(touch.px, touch.py, HOME_ICON_X, HOME_ICON_Y, ICON_SIZE, ICON_SIZE)) {
            return BOTTOM_ACTION_GO_HOME;
        }
    }

    return action;
}

static void draw_toolbar(void) {
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, TOOLBAR_HEIGHT, UI_COLOR_HEADER);
    ui_draw_icon_home(HOME_ICON_X, HOME_ICON_Y, ICON_SIZE, UI_COLOR_TEXT);
    ui_draw_icon_info(INFO_ICON_X, INFO_ICON_Y, ICON_SIZE, UI_COLOR_TEXT);
    ui_draw_icon_search(SEARCH_ICON_X, SEARCH_ICON_Y, ICON_SIZE, UI_COLOR_TEXT);
    ui_draw_icon_queue(QUEUE_ICON_X, QUEUE_ICON_Y, ICON_SIZE, UI_COLOR_TEXT);
    ui_draw_icon_gear(GEAR_ICON_X, GEAR_ICON_Y, ICON_SIZE, UI_COLOR_TEXT);
    ui_draw_icon_bug(BUG_ICON_X, BUG_ICON_Y, ICON_SIZE, UI_COLOR_TEXT);
}

static void draw_settings_screen(void) {
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, TOOLBAR_HEIGHT, UI_COLOR_HEADER);
    ui_draw_icon_bug(BUG_ICON_X, BUG_ICON_Y, ICON_SIZE, UI_COLOR_TEXT);

    if (showCancelButton) {
        ui_draw_button(BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT, "Save and Connect", saveButtonPressed, UI_BUTTON_PRIMARY);
        ui_draw_button(BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, "Cancel", cancelButtonPressed, UI_BUTTON_SECONDARY);
    } else {
        ui_draw_button(BUTTON_X, SAVE_BUTTON_Y_SINGLE, BUTTON_WIDTH, BUTTON_HEIGHT, "Save and Connect", saveButtonPressed, UI_BUTTON_PRIMARY);
    }
}

static void draw_rom_actions_screen(void) {
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
    draw_toolbar();

    const char *downloadLabel = romExists ? "Download Again" : "Download";
    ui_draw_button(BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT, downloadLabel, downloadButtonPressed, UI_BUTTON_PRIMARY);

    const char *queueLabel = romQueued ? "Remove from Queue" : "Add to Queue";
    UIButtonStyle queueStyle = romQueued ? UI_BUTTON_DANGER : UI_BUTTON_SECONDARY;
    ui_draw_button(BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, queueLabel, queueButtonPressed, queueStyle);
}

static void draw_downloading_screen(void) {
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
    ui_draw_button(BUTTON_X, SAVE_BUTTON_Y_SINGLE, BUTTON_WIDTH, BUTTON_HEIGHT, "Cancel Download", false, UI_BUTTON_DANGER);
}

static void draw_queue_screen(void) {
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
    draw_toolbar();

    if (queueItemCount > 0) {
        ui_draw_button(BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT, "Start Downloads", startDownloadsPressed, UI_BUTTON_PRIMARY);
        ui_draw_button(BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, "Clear Queue", clearQueuePressed, UI_BUTTON_DANGER);
    }
}

static void draw_queue_confirm_screen(void) {
    ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
    draw_toolbar();

    const char *prompt = "Are you sure?";
    float promptWidth = ui_get_text_width(prompt);
    ui_draw_text((SCREEN_BOTTOM_WIDTH - promptWidth) / 2, SAVE_BUTTON_Y_DUAL - UI_LINE_HEIGHT - UI_PADDING,
                 prompt, UI_COLOR_TEXT);

    ui_draw_button(BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT, "Clear Queue", confirmClearPressed, UI_BUTTON_DANGER);
    ui_draw_button(BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, "Cancel", cancelClearPressed, UI_BUTTON_SECONDARY);
}

void bottom_draw(void) {
    C2D_SceneBegin(bottomTarget);
    C2D_TargetClear(bottomTarget, UI_COLOR_BG);

    if (debuglog_is_visible()) {
        debuglog_draw();
    } else if (currentMode == BOTTOM_MODE_SETTINGS) {
        draw_settings_screen();
    } else if (currentMode == BOTTOM_MODE_ROM_ACTIONS) {
        draw_rom_actions_screen();
    } else if (currentMode == BOTTOM_MODE_DOWNLOADING) {
        draw_downloading_screen();
    } else if (currentMode == BOTTOM_MODE_QUEUE) {
        draw_queue_screen();
    } else if (currentMode == BOTTOM_MODE_QUEUE_CONFIRM) {
        draw_queue_confirm_screen();
    } else if (currentMode == BOTTOM_MODE_SEARCH_FORM) {
        ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
        draw_toolbar();
        search_form_draw();
    } else if (currentMode == BOTTOM_MODE_FOLDER_BROWSER) {
        ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
        draw_toolbar();

        char useLabel[280];
        if (folderName[0]) {
            snprintf(useLabel, sizeof(useLabel), "Use \"%s\"", folderName);
        } else {
            snprintf(useLabel, sizeof(useLabel), "Use Selected Folder");
        }
        ui_draw_button(BUTTON_X, SAVE_BUTTON_Y_DUAL, BUTTON_WIDTH, BUTTON_HEIGHT, useLabel, selectFolderPressed, UI_BUTTON_PRIMARY);
        ui_draw_button(BUTTON_X, CANCEL_BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, "Create New Folder", createFolderPressed, UI_BUTTON_SECONDARY);
    } else if (currentMode == BOTTOM_MODE_ABOUT) {
        ui_draw_rect(0, 0, SCREEN_BOTTOM_WIDTH, SCREEN_BOTTOM_HEIGHT, UI_COLOR_BG);
        draw_toolbar();

        float qrSize = 150.0f;
        float qrX = (SCREEN_BOTTOM_WIDTH - qrSize) / 2;
        float qrY = TOOLBAR_HEIGHT + (SCREEN_BOTTOM_HEIGHT - TOOLBAR_HEIGHT - qrSize) / 2;
        ui_draw_qr_code(qrX, qrY, qrSize);
    } else {
        draw_toolbar();
    }
}

bool bottom_check_cancel(void) {
    hidScanInput();
    u32 kDown = hidKeysDown();

    if (kDown & KEY_TOUCH) {
        touchPosition touch;
        hidTouchRead(&touch);
        if (ui_touch_in_rect(touch.px, touch.py, BUTTON_X, SAVE_BUTTON_Y_SINGLE, BUTTON_WIDTH, BUTTON_HEIGHT)) {
            return true;
        }
    }

    return false;
}
