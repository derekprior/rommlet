# Copilot Instructions for Rommlet

Rommlet is a Nintendo 3DS homebrew client for [RomM](https://github.com/rommapp/romm) servers, written in C using devkitPro/libctru/citro2d.

## Build Commands

```bash
make                        # Build .3dsx
make cia                    # Build .cia (installable title)
make clean                  # Clean build files
make format                 # Auto-format source with clang-format
make format-check           # Check formatting (used in CI)
```

CI builds with `make EXTRA_CFLAGS=-Werror` — all warnings are errors. Always build locally before pushing.

There are no tests or linters configured. The build is the only verification.

## Architecture

### State Machine & Navigation

`main.c` drives the app via an enum-based state machine (`AppState`). The main loop calls each screen's `_update()` and `_draw()` functions based on the current state, then transitions on the returned result enum.

States: `STATE_LOADING` → `STATE_SETTINGS` / `STATE_PLATFORMS` → `STATE_ROMS` → `STATE_ROM_DETAIL` → `STATE_SELECT_ROM_FOLDER` / `STATE_QUEUE` / `STATE_SEARCH_FORM` → `STATE_SEARCH_RESULTS` / `STATE_ABOUT`

Navigation uses a push/pop stack (`nav_push`/`nav_pop`/`nav_clear`, max depth 8). Push the current state before entering a new screen; pop on back. `nav_clear()` on "go home" prevents stale stack. Fallback on empty pop is `STATE_PLATFORMS`.

### Screen Pattern

Every screen module follows a consistent contract:

```c
void screen_init(...);                  // Set up screen state
ScreenResult screen_update(u32 kDown);  // Handle input, return action
void screen_draw(void);                 // Render frame
```

Each screen defines its own result enum (e.g., `PLATFORMS_SELECTED`, `ROMS_BACK`). Screens expose accessor functions (e.g., `roms_get_selected_index()`) rather than using out-parameters — the one exception is `platforms_update` which uses an out-parameter because `selectedPlatformIndex` is long-lived state used across multiple handlers.

The bottom screen is special — it's always rendered and returns `BottomAction` for global toolbar actions. Touch handling uses a data-driven `TouchButton` array pattern with a generic `handle_touch_buttons()` dispatcher.

To add a new screen: create the `_init/_update/_draw` functions in `source/screens/`, define a result enum, add a new `AppState`, and wire it into the main loop switch in `main.c`.

### Shared List Navigation

`listnav.h`/`listnav.c` provides a shared `ListNav` struct used by 5 scrollable list screens (platforms, roms, search results, queue, browser). Key functions: `listnav_set()`, `listnav_update()`, `listnav_visible_range()`, `listnav_draw_scroll_indicator()`, `listnav_on_load_more()`. The `visibleItems` field defaults to `UI_VISIBLE_ITEMS` (8) when set to 0; browser overrides it to 7 (path header takes a line).

### API Layer

`api.c` wraps HTTP requests to the RomM server using libctru's httpc. Key patterns:
- Functions return malloc'd structs; callers free with matching `api_free_*()` functions
- `setup_http_headers()` consolidates SSL, keepalive, User-Agent, Accept, and Authorization for all HTTP calls (including after redirects)
- RomM's filename field is `fs_name` (not `file_name`)
- Download URL format: `/api/roms/{id}/content/{fs_name}`
- Paginated responses use `items`, `offset`, `limit`, `total` fields
- `DownloadProgressCb` callback enables real-time progress rendering during downloads
- `SSLCOPT_DisableVerify` is correct — the 3DS has no usable CA store for homebrew

### Download Queue

Persistent queue at `sdmc:/3ds/rommlet/queue.txt` (tab-separated, one entry per line). Fields: `romId`, `platformId`, `platformSlug`, `fsName`, `name`. Queue saves on every mutation (add/remove/clear) and loads at startup. Empty queue deletes the file. Corrupt files (all lines malformed) are deleted with a log error.

### Logging

Leveled logging (`LOG_TRACE` through `LOG_FATAL`) with a subscriber pattern. Call `log_info()`, `log_debug()`, etc. from anywhere — messages broadcast to all registered subscribers. The debug log viewer (`debuglog.c`) registers as a subscriber and renders as a modal overlay on the bottom screen.

### Config

INI-format file at `sdmc:/3ds/rommlet/config.ini`. Main fields: `serverUrl`, `username`, `password`, `romFolder`. A `[platform_mappings]` section maps platform slugs to SD card folder names, cached in memory (max 64 entries). Settings are only saved via the bottom screen touch button (not d-pad).

## Conventions

### Naming

- Functions: `module_action()` — e.g., `platforms_update()`, `config_load()`, `queue_add()`
- Enums: `SCREAMING_SNAKE_CASE` — e.g., `STATE_PLATFORMS`, `ROMS_SELECTED`
- Struct typedefs: `PascalCase` — e.g., `Platform`, `RomDetail`, `QueueEntry`
- Constants: `PREFIX_NAME` — e.g., `UI_PADDING`, `QUEUE_MAX_ENTRIES`

### Module Encapsulation

Each `.c` file uses `static` variables for module-private state — no shared globals. Modules expose `_init()` / `_exit()` lifecycle functions.

### String Safety

All string operations use `snprintf` — no `strcpy` or `strncpy` anywhere. Use `snprintf(dst, sizeof(dst), "%s", src)` for copies.

### Memory

API functions that allocate memory always have a corresponding `api_free_*()` function. Callers own returned memory.

### Formatting

Code is formatted with clang-format (`.clang-format` in repo root): LLVM base, 4-space indent, 120 column limit, `AllowShortIfStatementsOnASingleLine: AllIfsAndElse`. Run `make format` before committing. CI enforces with `make format-check`.

### 3DS-Specific Notes

- `SwkbdState` and output buffer must be `static` (not stack-allocated) for CIA mode compatibility — the APT applet transition requires stable memory
- `bottom_check_cancel()` calls `hidScanInput()` during the download progress callback — this is safe because the progress callback IS the frame loop during downloads (the main loop is not running)
- Font is loaded with `CFG_REGION_USA` — works on all regions for Latin text
- Cleanup order in `main()` is reverse of init order

### RomM API Compatibility

Requires RomM 4.6.0+. Uses `platform_ids` parameter (not `platform_id`).
