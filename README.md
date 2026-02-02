# Rommlet

A Nintendo 3DS homebrew app for browsing ROMs on a [RomM](https://github.com/rommapp/romm) server.

## Features

- Browse platforms available on your RomM server
- Browse ROMs within each platform
- Paginated ROM lists for large collections
- In-app settings with on-screen keyboard
- HTTP Basic Auth support

## Requirements

### For Building

- [devkitPro](https://devkitpro.org/wiki/Getting_Started) with devkitARM
- 3ds-dev packages: `(dkp-)pacman -S 3ds-dev`
- [bannertool](https://github.com/Steveice10/bannertool) and [makerom](https://github.com/3DSGuy/Project_CTR) (for CIA builds)
- [librsvg](https://wiki.gnome.org/Projects/LibRsvg) for icon conversion: `brew install librsvg`

Or use the included devcontainer for GitHub Codespaces.

### For Running

- Nintendo 3DS with custom firmware (Luma3DS recommended)
- Homebrew Launcher (for .3dsx) or FBI (for .cia)
- WiFi connection to your RomM server

## Building

### Option 1: GitHub Codespaces (Recommended)

No local setup required! Build in the cloud with a pre-configured environment.

1. Click the green **Code** button on GitHub
2. Select **Codespaces** → **Create codespace on main**
3. Wait for the container to build (~2-3 minutes first time)
4. In the terminal, run:
   ```bash
   make
   ```
5. Download `rommlet.3dsx` from the file explorer

### Option 2: Build Locally

```bash
# Build .3dsx (for Homebrew Launcher)
make

# Build .cia (installable title)
make cia

# Clean build files
make clean
```

## Installation

### Using .3dsx (Homebrew Launcher)

1. Copy `rommlet.3dsx` to `/3ds/` on your SD card
2. Launch via Homebrew Launcher

### Using .cia (Installable)

1. Copy `rommlet.cia` to your SD card
2. Install with FBI or similar
3. Launch from Home Menu

## Usage

1. On first launch, you'll be prompted to configure the server
2. Enter your RomM server URL (e.g., `http://192.168.1.100:8080`)
3. Optionally enter username/password if your server requires authentication
4. Browse platforms and ROMs!

### Controls

| Button | Action |
|--------|--------|
| D-Pad | Navigate |
| A | Select / Edit |
| B | Back |
| L/R | Page up/down |
| X | Refresh |
| SELECT | Settings |
| START | Exit |

## Configuration

Settings are stored at `/3ds/rommlet/config.ini` on your SD card.

## Development

The app is written in C using:

- **libctru** - Core 3DS library for HTTP, input, filesystem
- **citro2d** - 2D graphics library
- **cJSON** - JSON parsing (MIT license, vendored)

### Project Structure

```
source/
├── main.c              # Entry point, state machine
├── config.c/h          # Settings load/save
├── api.c/h             # RomM API wrapper
├── ui.c/h              # UI drawing helpers
├── cJSON/              # JSON parser (vendored)
└── screens/
    ├── settings.c/h    # Settings screen
    ├── platforms.c/h   # Platform list
    └── roms.c/h        # ROM list
```

## RomM API Compatibility

Requires RomM 4.6.0 or newer. Key API differences from earlier versions:

- Uses `platform_ids` parameter (not `platform_id`)
- Paginated ROM responses with `items`, `offset`, `limit`, `total`

## License

MIT License - see [LICENSE](LICENSE) for details.

## Credits

- [RomM](https://github.com/rommapp/romm) - ROM management server
- [devkitPro](https://devkitpro.org/) - 3DS homebrew toolchain
- [cJSON](https://github.com/DaveGamble/cJSON) - JSON parser
