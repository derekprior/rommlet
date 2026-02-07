# Contributing to Rommlet

Contributions are welcome! Whether it's a bug fix, new feature, or improvement to documentation, we'd love your help.

## Getting Started

The fastest way to get a development environment is with GitHub Codespaces or a Dev Container:

1. **Codespaces**: Click **Code** → **Codespaces** → **Create codespace on main** — everything is pre-configured
2. **Dev Container**: Open the repo in VS Code with the [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) and reopen in the container

Both options give you the full devkitPro toolchain, makerom, bannertool, and clang-format ready to go. No local setup required.

## Build Commands

```bash
make                        # Build .3dsx
make cia                    # Build .cia (installable title)
make clean                  # Clean build files
make format                 # Auto-format source with clang-format
make format-check           # Check formatting (used in CI)
```

CI builds with `make EXTRA_CFLAGS=-Werror` — all warnings are errors. Always build and format before pushing:

```bash
make format && make EXTRA_CFLAGS=-Werror
```

There are no tests. The build and format check are the only verification, so please test on hardware or in an emulator when possible.

## Architecture Overview

See [`.github/copilot-instructions.md`](.github/copilot-instructions.md) for a detailed overview of the application architecture, conventions, and patterns used throughout the codebase.

## Submitting Changes

1. Fork the repo and create a branch from `main`
2. Make your changes
3. Run `make format && make EXTRA_CFLAGS=-Werror` to ensure a clean build
4. Open a pull request with a clear description of what you changed and why
