# Rommlet

A Nintendo 3DS homebrew client for [RomM](https://github.com/rommapp/romm). Browse, search, and download ROMs from your RomM server directly to your 3DS.

## Features

- **Browse** platforms and ROMs on your RomM server with paginated lists
- **Search** for ROMs across all platforms or filter by specific ones
- **Download** ROMs directly to your SD card with a progress indicator
- **Download queue** to batch multiple ROMs for download
- **Folder management** — choose or create destination folders per platform
- **Touchscreen UI** with toolbar icons for quick navigation between home, search, queue, and settings
- **D-Pad + button controls** for navigating lists and selecting ROMs

## Requirements

- Nintendo 3DS with custom firmware (Luma3DS recommended)
- Homebrew Launcher (for .3dsx) or FBI (for .cia)
- WiFi connection to your RomM server (v4.6.0 or newer)

## Installation

### Using .3dsx (Homebrew Launcher)

1. Copy `rommlet.3dsx` to `/3ds/` on your SD card
2. Launch via Homebrew Launcher

### Using .cia (Installable)

1. Copy `rommlet.cia` to your SD card
2. Install with FBI or similar
3. Launch from Home Menu

## Usage

On first launch, you'll be prompted to configure your server connection:

1. Enter your RomM server URL (e.g., `http://192.168.1.100:8080`)
2. Optionally enter username and password if your server requires authentication
3. Choose a root folder on your SD card for ROM storage

Once connected, browse platforms and ROMs using the D-Pad and A button. Use the touchscreen toolbar to access search, the download queue, and settings.

### Configuration

Settings are stored at `/3ds/rommlet/config.ini` on your SD card.

## Building from Source

### GitHub Codespaces (Recommended)

1. Click **Code** → **Codespaces** → **Create codespace on main**
2. Run `make` for .3dsx or `make cia` for .cia
3. Download the built file from the file explorer

### Local Build

Requires [devkitPro](https://devkitpro.org/wiki/Getting_Started) with devkitARM and 3ds-dev packages. For CIA builds, also requires [bannertool](https://github.com/Steveice10/bannertool) and [makerom](https://github.com/3DSGuy/Project_CTR).

```bash
make          # Build .3dsx
make cia      # Build .cia
make clean    # Clean build files
```

## License

MIT License - see [LICENSE](LICENSE) for details.

## Credits

- [RomM](https://github.com/rommapp/romm) - ROM management server
- [devkitPro](https://devkitpro.org/) - 3DS homebrew toolchain
- [cJSON](https://github.com/DaveGamble/cJSON) - JSON parser
