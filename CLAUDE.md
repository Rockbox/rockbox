# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Rockbox is an open source firmware replacement for digital audio players. It supports a wide variety of hardware targets including iPod, Sansa, iriver, Gigabeat, and many others, as well as SDL-based simulators for development.

## Build Commands

### Prerequisites
- Cross-compiler toolchain (ARM, MIPS, or M68K depending on target)
- Run `tools/rockboxdev.sh` to download and build the required cross-compilers
- Perl must be in PATH
- SDL2 for simulator builds (`sdl2-config` must be available)
- On macOS: requires GNU sed (`gsed`) and GNU readlink (`greadlink`)

### Building
```bash
mkdir build && cd build
../tools/configure    # Interactive - select target device
make                  # Build firmware
make zip              # Create distribution package
```

The configure script is interactive and prompts for:
- Target device (native hardware or simulator)
- Build type (normal, debug, etc.)

For simulator builds, select the SDL Application or simulator option for your platform.

### Building for Multiple Targets
Create separate build directories for each target:
```bash
mkdir build-sim && cd build-sim && ../tools/configure
mkdir build-ipod && cd build-ipod && ../tools/configure
```

### Docker Build (iPod Video)
For reproducible builds without installing toolchains locally:
```bash
make build   # Build firmware
make clean   # Remove output
```
Artifacts are placed in `output/` (rockbox.ipod and rockbox.zip).

## Testing

There is no formal unit test framework. Testing is done through:

1. **Test plugins** - Built into the firmware, run on device or simulator:
   - `test_codec.c` - Codec performance testing
   - `test_fps.c` - Frame rate benchmarking
   - `test_mem.c` - Memory allocation testing
   - `test_disk.c` - Disk I/O testing
   - `test_gfx.c` - Graphics rendering tests

2. **Warble** - Standalone codec testing tool in `lib/rbcodec/test/`

3. **Simulator** - Build the SDL simulator target to test UI and playback without hardware

## Code Architecture

### Directory Structure
- **firmware/** - Low-level code: kernel, drivers, HAL, filesystem
- **apps/** - Application layer: UI, menus, playback, recording
- **lib/** - Shared libraries (codecs, skin parser, fixed-point math)
- **bootloader/** - Device-specific bootloaders
- **tools/** - Build system and utilities
- **uisimulator/** - SDL-based UI simulator support

### Key Subsystems

**Firmware Layer** (`firmware/`):
- `kernel/` - Threading (cooperative), queues, mutexes, semaphores
- `drivers/` - LCD, buttons, audio, USB stack
- `common/` - FAT filesystem, file operations
- `target/` - Platform-specific code organized by CPU architecture
- `export/` - Public headers for firmware APIs

**Application Layer** (`apps/`):
- `main.c` - System initialization and main loop
- `playlist.c`, `buffering.c` - Playback infrastructure
- `tagcache.c` - Music metadata database
- `plugins/` - Loadable plugins (games, tools, viewers)
- `recorder/` - Recording functionality

**Codec Library** (`lib/rbcodec/`):
- `codecs/` - Individual codec implementations (MP3, FLAC, AAC, etc.)
- `dsp/` - Digital signal processing (EQ, replaygain)
- `metadata/` - File metadata parsers

### Threading Model
Multi-threaded cooperative kernel with main threads for:
- UI/main thread
- Codec thread (audio decoding)
- Audio thread (PCM output)
- Voice thread (accessibility)
- Recording thread

Inter-thread communication via event queues.

### Plugin Architecture
Plugins are dynamically loaded modules that access firmware through a function pointer table (`struct plugin_api`). See `docs/PLUGIN_API` for the complete API.

### Configuration System
Compile-time configuration via preprocessor macros (`CONFIG_*`). Device capabilities are defined in header files under `firmware/export/config/`.

## Coding Conventions

- **Language**: C only (assembly when absolutely necessary)
- **Identifiers**: lowercase with underscores; UPPERCASE for macros and enum constants
- **No typedefs** for structs/enums
- **Indentation**: 4 spaces (no tabs)
- **Line length**: Under 80 columns
- **Comments**: C-style only (`/* */`)
- **Line endings**: Unix (LF only)

When modifying existing files, follow the style already present in that file.

## Key Files

- `tools/configure` - Build configuration script
- `firmware/export/config.h` - Master configuration header
- `apps/main.c` - Application entry point
- `docs/CONTRIBUTING` - Full contribution guidelines
- `docs/PLUGIN_API` - Plugin development reference
