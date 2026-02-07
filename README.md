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

Download the [latest release](https://github.com/derekprior/rommlet/releases/latest) and unzip it to the `/3ds/` folder on your SD card. From there you have two options:

- **Homebrew Launcher** — Launch `rommlet.3dsx` directly from the Homebrew Launcher
- **Home Menu (CIA)** — Install `rommlet.cia` using FBI or a similar title manager, then launch from the Home Menu

### Enabling Sound

Rommlet plays UI sound effects using the 3DS DSP hardware. This requires the DSP firmware file `dspfirm.cdc` on your SD card. Sound is optional — the app works fine without it.

If you don't already have this file, dump it using the Rosalina menu in Luma3DS:

1. Press **L + D-Pad Down + Select** to open the Rosalina menu
2. Select **Miscellaneous options...**
3. Select **Dump DSP firmware**
4. Press **B** to return, then **B** again to close the menu

This only needs to be done once. The file is saved to `sdmc:/3ds/dspfirm.cdc` and is used by all homebrew that supports audio.

## Usage

On first launch, you'll be prompted to configure your server connection:

1. Enter your RomM server URL (e.g., `http://192.168.1.100:8080`)
2. Optionally enter username and password if your server requires authentication
3. Choose a root folder on your SD card for ROM storage

Once connected, browse platforms and ROMs using the D-Pad and A button. Use the touchscreen toolbar to access search, the download queue, and settings.

### Configuration

Settings are stored at `/3ds/rommlet/config.ini` on your SD card.

### Security

Your RomM username and password are stored in plain text in `config.ini` on your SD card. Authentication uses HTTP Basic Auth, which transmits credentials unencrypted unless your server is available via HTTPS. Use a unique password for your RomM server and rely on other security measures (network isolation, VPN, etc.) if you are concerned about access.

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
