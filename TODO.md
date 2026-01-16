Building this project is probably quite fussy.

Improve the portability and reliability of this project's build process by:
* creating a Dockerfile that stands up an ideal build machine for this project
* adding a build command that builds the Rockbox file(s) using the aforementioned Dockerfile
* neatly outputs all relevant build artifacts to a gitignored build directory
* include a clean command (or update the existing one) to clean up the build dir

## Clarified Requirements

### Target Device
- iPod Video 5.5th generation (30GB/60GB/80GB models)
- Configure option: `22|ipodvideo`
- Target macro: `IPOD_VIDEO`
- Output file: `rockbox.ipod` (plus `rockbox.zip` containing the full installation package)

### Toolchain
- ARM cross-compiler: `arm-elf-eabi-gcc` version 9.5.0
- Binutils version 2.38
- Built via `tools/rockboxdev.sh` with option "A" (ARM)
- The toolchain should be pre-installed in the Docker image for faster subsequent builds

### Build System Dependencies
Required packages for building the toolchain:
- gcc, g++, make, patch, automake, libtool, autoconf, flex, bison
- xz, bzip2, gzip (for archive extraction)
- texinfo (provides makeinfo)
- perl (required by configure script)
- wget or curl (for downloading toolchain sources)
- libgmp-dev, libmpfr-dev, libmpc-dev, libisl-dev (GCC build dependencies)

### Build Artifacts
Only output what's needed to install on the iPod:
- `rockbox.zip` - The main installation package (unzip to iPod root)
- `rockbox.ipod` - The firmware binary

### Build Wrapper
Use existing Makefile conventions in the repository. If modifying any Makefiles, update relevant documentation (CLAUDE.md, docs/README, etc.).