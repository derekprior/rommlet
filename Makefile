#---------------------------------------------------------------------------------
# Rommlet - RomM Client for Nintendo 3DS
# Simplified build system with vendored lpp-3ds
#---------------------------------------------------------------------------------
# Prerequisites:
#   - devkitPro with devkitARM (export DEVKITARM=/path/to/devkitARM)
#   - 3ds-dev package: (dkp-)pacman -S 3ds-dev
#   - bannertool and makerom in PATH
#
# Usage:
#   make          - Build everything (3dsx + cia)
#   make 3dsx     - Build .3dsx only
#   make cia      - Build .cia only
#   make clean    - Clean build files
#---------------------------------------------------------------------------------

.PHONY: all 3dsx cia clean romfs lpp-3ds help

#---------------------------------------------------------------------------------
# Project Configuration
#---------------------------------------------------------------------------------
APP_TITLE     := Rommlet
APP_AUTHOR    := Derek Prior
APP_DESC      := RomM Client for 3DS
APP_VERSION   := 1.0.0

# Unique ID for the CIA (hex, 5 digits max)
APP_UNIQUE_ID := 0xRMMLT
APP_PRODUCT   := CTR-P-RMLT

#---------------------------------------------------------------------------------
# Directories
#---------------------------------------------------------------------------------
ROMFS_DIR     := romfs
BUILD_DIR     := build
VENDOR_DIR    := vendor/lpp-3ds

#---------------------------------------------------------------------------------
# Source files
#---------------------------------------------------------------------------------
LUA_FILES     := index.lua config.lua api.lua ui.lua json.lua
LUA_SCREENS   := screens/settings.lua screens/platforms.lua screens/roms.lua

#---------------------------------------------------------------------------------
# Tools (these come with devkitPro 3ds-dev)
#---------------------------------------------------------------------------------
MAKEROM       := makerom
BANNERTOOL    := bannertool
RSVG_CONVERT  := rsvg-convert

#---------------------------------------------------------------------------------
# Output files
#---------------------------------------------------------------------------------
OUTPUT_3DSX   := $(BUILD_DIR)/$(APP_TITLE).3dsx
OUTPUT_CIA    := $(BUILD_DIR)/$(APP_TITLE).cia
OUTPUT_ELF    := $(VENDOR_DIR)/lpp-3ds.elf
OUTPUT_SMDH   := $(BUILD_DIR)/$(APP_TITLE).smdh

#---------------------------------------------------------------------------------
# Default: build both 3dsx and cia
#---------------------------------------------------------------------------------
all: 3dsx cia
	@echo ""
	@echo "============================================"
	@echo "  Build Complete!"
	@echo "============================================"
	@echo ""
	@echo "  $(OUTPUT_3DSX)"
	@echo "  $(OUTPUT_CIA)"
	@echo ""

#---------------------------------------------------------------------------------
# Build 3DSX (for Homebrew Launcher)
#---------------------------------------------------------------------------------
3dsx: romfs lpp-3ds $(OUTPUT_SMDH)
	@mkdir -p $(BUILD_DIR)
	@cp $(OUTPUT_ELF) $(BUILD_DIR)/$(APP_TITLE).elf
	@3dsxtool $(BUILD_DIR)/$(APP_TITLE).elf $(OUTPUT_3DSX) \
		--smdh=$(OUTPUT_SMDH) \
		--romfs=$(ROMFS_DIR)
	@echo "Built: $(OUTPUT_3DSX)"

#---------------------------------------------------------------------------------
# Build CIA (installable title)
#---------------------------------------------------------------------------------
cia: romfs lpp-3ds meta/banner.bin meta/icon.bin
	@mkdir -p $(BUILD_DIR)
	$(MAKEROM) -f cia -o $(OUTPUT_CIA) \
		-elf $(OUTPUT_ELF) \
		-rsf meta/rommlet.rsf \
		-icon meta/icon.bin \
		-banner meta/banner.bin \
		-DAPP_TITLE="$(APP_TITLE)" \
		-DAPP_PRODUCT_CODE="$(APP_PRODUCT)" \
		-DAPP_UNIQUE_ID="$(APP_UNIQUE_ID)" \
		-DROMFS_ROOT="$(ROMFS_DIR)"
	@echo "Built: $(OUTPUT_CIA)"

#---------------------------------------------------------------------------------
# Build lpp-3ds from vendored source
#---------------------------------------------------------------------------------
lpp-3ds: $(OUTPUT_ELF)

$(OUTPUT_ELF):
	@echo "Building lpp-3ds..."
	$(MAKE) -C $(VENDOR_DIR)

#---------------------------------------------------------------------------------
# Copy Lua scripts to romfs
#---------------------------------------------------------------------------------
romfs: $(ROMFS_DIR)/index.lua

$(ROMFS_DIR)/index.lua: $(LUA_FILES) $(LUA_SCREENS)
	@echo "Copying Lua scripts to romfs..."
	@mkdir -p $(ROMFS_DIR)/screens
	@cp $(LUA_FILES) $(ROMFS_DIR)/
	@cp $(LUA_SCREENS) $(ROMFS_DIR)/screens/
	@touch $(ROMFS_DIR)/index.lua

#---------------------------------------------------------------------------------
# Generate SMDH (icon metadata for 3DSX)
#---------------------------------------------------------------------------------
$(OUTPUT_SMDH): meta/icon.png
	@mkdir -p $(BUILD_DIR)
	$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" -l "$(APP_DESC)" -p "$(APP_AUTHOR)" \
		-i meta/icon.png -o $(OUTPUT_SMDH)

#---------------------------------------------------------------------------------
# Generate banner.bin and icon.bin for CIA
#---------------------------------------------------------------------------------
meta/banner.bin: meta/banner.png
	@echo "Creating banner.bin..."
	$(BANNERTOOL) makebanner -i meta/banner.png -o meta/banner.bin

meta/icon.bin: meta/icon.png
	@echo "Creating icon.bin..."
	$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" -l "$(APP_DESC)" -p "$(APP_AUTHOR)" \
		-i meta/icon.png -o meta/icon.bin

#---------------------------------------------------------------------------------
# Convert SVG to PNG (if needed)
#---------------------------------------------------------------------------------
meta/icon.png: meta/icon.svg
	$(RSVG_CONVERT) -w 48 -h 48 $< -o $@

meta/banner.png: meta/banner.svg
	$(RSVG_CONVERT) -w 256 -h 128 $< -o $@

#---------------------------------------------------------------------------------
# Clean
#---------------------------------------------------------------------------------
clean:
	@echo "Cleaning Rommlet build..."
	rm -rf $(BUILD_DIR)
	rm -rf $(ROMFS_DIR)
	rm -f meta/banner.bin meta/icon.bin meta/icon.png meta/banner.png
	@echo "Cleaning lpp-3ds build..."
	$(MAKE) -C $(VENDOR_DIR) clean || true
	@echo "Clean complete."

#---------------------------------------------------------------------------------
# Distribution package (for users without devkitPro)
#---------------------------------------------------------------------------------
dist: all
	@mkdir -p dist
	@cp $(OUTPUT_3DSX) dist/
	@cp $(OUTPUT_CIA) dist/
	@cp README.md dist/
	@echo "Distribution package created in dist/"

#---------------------------------------------------------------------------------
# Help
#---------------------------------------------------------------------------------
help:
	@echo ""
	@echo "Rommlet Build System"
	@echo "===================="
	@echo ""
	@echo "Prerequisites:"
	@echo "  - devkitPro with devkitARM"
	@echo "  - 3ds-dev package: (dkp-)pacman -S 3ds-dev"
	@echo "  - bannertool and makerom in PATH"
	@echo "  - librsvg (for SVG icons): brew install librsvg"
	@echo ""
	@echo "Targets:"
	@echo "  make        Build both .3dsx and .cia"
	@echo "  make 3dsx   Build .3dsx only (Homebrew Launcher)"
	@echo "  make cia    Build .cia only (installable)"
	@echo "  make dist   Create distribution package"
	@echo "  make clean  Remove all build artifacts"
	@echo "  make help   Show this help"
	@echo ""
