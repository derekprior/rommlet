# Copilot Instructions for Rommlet

Rommlet is a Nintendo 3DS homebrew client for [RomM](https://github.com/rommapp/romm) servers, written in C using devkitPro/libctru/citro2d.

## Build Commands

```bash
make                        # Build .3dsx
make cia                    # Build .cia (installable title)
make clean                  # Clean build files
```

CI builds with `make EXTRA_CFLAGS=-Werror` — all warnings are errors. Always build locally before pushing.

There are no tests or linters configured.

## Architecture

### State Machine

`main.c` drives the app via an enum-based state machine (`AppState`). The main loop calls each screen's `_update()` and `_draw()` functions based on the current state, then transitions on the returned result enum.

States: `STATE_LOADING` → `STATE_SETTINGS` / `STATE_PLATFORMS` → `STATE_ROMS` → `STATE_ROM_DETAIL` → `STATE_SELECT_ROM_FOLDER` / `STATE_QUEUE`

### Screen Pattern

Every screen module follows a consistent contract:

```c
void screen_init(...);                              // Set up screen state
ScreenResult screen_update(u32 kDown, int *out);    // Handle input, return action
void screen_draw(void);                             // Render frame
```

Each screen defines its own result enum (e.g., `PLATFORMS_SELECTED`, `ROMS_BACK`). The bottom screen is special — it's always rendered and returns `BottomAction` for global toolbar actions.

To add a new screen: create the `_init/_update/_draw` functions, define a result enum, add a new `AppState`, and wire it into the main loop switch.

### API Layer

`api.c` wraps HTTP requests to the RomM server using libctru's httpc. Key patterns:
- Functions return malloc'd structs; callers free with matching `api_free_*()` functions
- RomM's filename field is `fs_name` (not `file_name`)
- Download URL format: `/api/roms/{id}/content/{fs_name}`
- Paginated responses use `items`, `offset`, `limit`, `total` fields
- `DownloadProgressCb` callback enables real-time progress rendering

### Logging

Leveled logging (`LOG_TRACE` through `LOG_FATAL`) with a subscriber pattern. Call `log_info()`, `log_debug()`, etc. from anywhere — messages broadcast to all registered subscribers. The bottom screen registers as a subscriber to display log messages.

### Config

INI-format file at `sdmc:/3ds/rommlet/config.ini`. Main fields: `serverUrl`, `username`, `password`, `romFolder`. A `[platform_mappings]` section maps platform slugs to SD card folder names, cached in memory (max 64 entries).

## Conventions

### Naming

- Functions: `module_action()` — e.g., `platforms_update()`, `config_load()`, `queue_add()`
- Enums: `SCREAMING_SNAKE_CASE` — e.g., `STATE_PLATFORMS`, `ROMS_SELECTED`
- Struct typedefs: `PascalCase` — e.g., `Platform`, `RomDetail`, `QueueEntry`
- Constants: `PREFIX_NAME` — e.g., `UI_PADDING`, `QUEUE_MAX_ENTRIES`

### Module Encapsulation

Each `.c` file uses `static` variables for module-private state — no shared globals. Modules expose `_init()` / `_exit()` lifecycle functions.

### Memory

API functions that allocate memory always have a corresponding `api_free_*()` function. Callers own returned memory.

### RomM API Compatibility

Requires RomM 4.6.0+. Uses `platform_ids` parameter (not `platform_id`).
