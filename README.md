# Rommlet

A Nintendo 3DS homebrew app for browsing ROMs on a [RomM](https://github.com/rommapp/romm) server.

## Features

- Browse platforms available on your RomM server
- Browse ROMs within each platform
- Paginated ROM lists for large collections
- In-app settings with on-screen keyboard

## Requirements

- Nintendo 3DS with custom firmware (Luma3DS recommended)
- [lpp-3ds](https://github.com/Rinnegatamante/lpp-3ds) (Lua Player Plus 3DS)
- A running RomM server (v4.6.0 or newer)
- Wi-Fi connection on your 3DS

## Installation

### Option 1: Quick Install (3DSX)

1. Download [lpp-3ds](https://www.rinnegatamante.eu/lpp-nightly.php) nightly build
2. Copy the `lpp-3ds.3dsx` file to `/3ds/rommlet/` on your SD card
3. Copy all `.lua` files from this project to `/3ds/rommlet/`:
   - `index.lua`
   - `config.lua`
   - `api.lua`
   - `ui.lua`
   - `json.lua`
4. Create a `screens` folder inside `/3ds/rommlet/`
5. Copy the screen files to `/3ds/rommlet/screens/`:
   - `settings.lua`
   - `platforms.lua`
   - `roms.lua`

Your SD card structure should look like:
```
/3ds/
  rommlet/
    lpp-3ds.3dsx
    index.lua
    config.lua
    api.lua
    ui.lua
    json.lua
    screens/
      settings.lua
      platforms.lua
      roms.lua
```

### Option 2: Build with GitHub Codespaces (Recommended)

No local setup required! Build in the cloud with a pre-configured environment.

1. Click the green **Code** button on GitHub
2. Select **Codespaces** → **Create codespace on main**
3. Wait for the container to build (~2-3 minutes first time)
4. In the terminal, run:
   ```bash
   make
   ```
5. Download `build/Rommlet.cia` and `build/Rommlet.3dsx` from the file explorer

The devcontainer includes all required tools: devkitARM, bannertool, makerom, and librsvg.

### Option 3: Build Locally

If you prefer to build on your own machine:

**Prerequisites:**
- [devkitPro](https://devkitpro.org/wiki/Getting_Started) with devkitARM
- 3ds-dev package: `(dkp-)pacman -S 3ds-dev`
- `bannertool` and `makerom` in your PATH
- `librsvg` for icon conversion: `brew install librsvg` (macOS) or `apt install librsvg2-bin` (Linux)

**Build:**

```bash
git clone https://github.com/derekprior/rommlet.git
cd rommlet
make
```

That's it! The build system uses vendored lpp-3ds source code, so no additional setup is needed.

**Output:**
- `build/Rommlet.3dsx` - For Homebrew Launcher
- `build/Rommlet.cia` - Installable via FBI

**Other targets:**
```bash
make 3dsx   # Build .3dsx only
make cia    # Build .cia only
make dist   # Create distribution package
make clean  # Clean build artifacts
make help   # Show all options
```

## First Run

1. Launch the Homebrew Launcher on your 3DS
2. Select "rommlet" from the list
3. On first run, you'll see the Settings screen
4. Enter your RomM server details:
   - **Server URL**: Your RomM server address (e.g., `http://192.168.1.100:3000`)
   - **Username**: Your RomM username
   - **Password**: Your RomM password
5. Press START to save settings and continue

## Controls

### Platforms Screen
- **D-Pad Up/Down**: Navigate platform list
- **A**: Select platform (view ROMs)
- **Y**: Refresh platform list
- **SELECT**: Open settings
- **START**: Exit app

### ROMs Screen
- **D-Pad Up/Down**: Navigate ROM list
- **D-Pad Left/Right**: Previous/Next page
- **Y**: Refresh current page
- **B**: Back to platforms

### Settings Screen
- **D-Pad Up/Down**: Navigate fields
- **A**: Edit selected field
- **START**: Save settings and continue
- **B**: Cancel (if already configured)

## Troubleshooting

### "Failed to connect to server"
- Ensure your 3DS Wi-Fi is enabled and connected
- Verify the server URL is correct (include `http://` and port if needed)
- Check that your RomM server is accessible from your network

### "Authentication failed"
- Double-check your username and password
- Ensure the user has API access permissions in RomM

### No platforms showing
- Press Y to refresh
- Check server connection in settings
- Verify your RomM server has scanned platforms

## Technical Notes

- Uses HTTP Basic Authentication for RomM API
- Compatible with RomM v4.6.0+ API
- Configuration stored at `/3ds/rommlet/config.txt`

## Building

```bash
make          # Build both .3dsx and .cia
make 3dsx     # Build .3dsx only
make cia      # Build .cia only  
make dist     # Create distribution package
make clean    # Clean build artifacts
make help     # Show all options
```

The build uses vendored lpp-3ds source in `vendor/lpp-3ds/`.

## License

This project is licensed under the **GNU General Public License v3.0** (GPL-3.0).

This is required because Rommlet includes [lpp-3ds](https://github.com/Rinnegatamante/lpp-3ds), which is GPL v3 licensed.

See [LICENSE](LICENSE) for the full text.

## Credits

- [RomM](https://github.com/rommapp/romm) - ROM Manager
- [lpp-3ds](https://github.com/Rinnegatamante/lpp-3ds) - Lua Player Plus 3DS by Rinnegatamante
