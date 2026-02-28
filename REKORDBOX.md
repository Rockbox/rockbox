# Rockbox + Rekordbox for iPod 5G/5.5G

A custom Rockbox build that lets your iPod Video live a double life: plug it into a Pioneer CDJ as a Rekordbox USB source, then unplug and use it as a standalone music player with Rockbox — no syncing scripts, no extra software, no compromise.

---

## How it works

When you export your library from Rekordbox to a USB drive, Pioneer creates a binary database at `/PIONEER/rekordbox/export.pdb` alongside your audio files in `/PIONEER/Contents/`. This build adds a parser for that database directly into Rockbox's core. On every boot, Rockbox checks whether the PDB has changed since the last import. If it has, it reads the track metadata (title, artist, album, genre, BPM, rating) and populates the native Rockbox database — so you can browse your Rekordbox library through all the usual Rockbox menus.

It also generates `.m3u8` playlist files in `/Playlists/Rekordbox/` from your Rekordbox playlists, and re-imports automatically after every USB sync session.

---

## Requirements

- **iPod**: 5th generation (Video, 30GB) or 5.5th generation (Video, 80GB)
- **Rekordbox**: version 6.x or 7.x on macOS or Windows
- **Drive format**: FAT32 — required by both Rockbox and Pioneer CDJs
- **This build**: compiled from this repository (see [Building](#building))

> **5G vs 5.5G?** Both are "iPod Video." The 5.5G (late 2006) added the 80GB drive option and a slightly brighter screen. Same PP5022 chipset, same Rockbox support. This build works on both.

---

## Drive preparation

Pioneer CDJs require FAT32. iPods sold in most markets ship formatted correctly, but check before proceeding.

**On macOS:**
```
diskutil list
diskutil eraseDisk FAT32 IPOD MBRFormat /dev/diskN
```

**On Windows:**
- Open Disk Management, right-click the iPod partition → Format → FAT32
- For drives > 32GB, use [Rufus](https://rufus.ie) or `fat32format`

---

## Installing Rockbox

### 1. Install Rockbox Utility

Download [Rockbox Utility](https://www.rockbox.org/wiki/RockboxUtility) for your OS. It automates bootloader installation and file copying.

> **Do not use the official Rockbox Utility build for this.** It will install the unmodified Rockbox firmware. You need to install the bootloader with Rockbox Utility, then manually copy the custom `.rockbox` folder from this build.

### 2. Install the bootloader

1. Connect your iPod via USB
2. Open Rockbox Utility → **Bootloader** → **Install**
3. Follow the prompts — it patches the iPod firmware partition so Rockbox boots instead of (or alongside) Apple OS

### 3. Copy the custom firmware

After building (see [Building](#building)), copy the output `.rockbox` folder to the root of your iPod:

```
/               ← iPod root
├── .rockbox/   ← Rockbox firmware, codecs, themes, database
│   ├── rockbox.ipod
│   ├── codecs/
│   └── ...
```

The iPod now boots Rockbox.

---

## Exporting from Rekordbox

1. In Rekordbox, go to **File → Export → Export to Removable Device** (or drag tracks/playlists to the device panel)
2. Select your iPod as the export target
3. Choose which playlists or crates to export
4. Click **Export**

Rekordbox creates:

```
/PIONEER/
├── rekordbox/
│   └── export.pdb          ← binary database (tracks, playlists, metadata)
└── Contents/
    ├── 01/
    │   ├── track001.mp3
    │   └── track002.flac
    ├── 02/
    └── ...
```

Your audio files live in `/PIONEER/Contents/` in sequentially numbered subdirectories. The `export.pdb` file references them with paths like `/Contents/01/track001.mp3` — this build prepends `/PIONEER` to resolve them to `/PIONEER/Contents/01/track001.mp3` on the device.

---

## First boot after export

On the first boot after an export (or after any re-export that changes the PDB):

1. Rockbox detects that `/PIONEER/rekordbox/export.pdb` is newer than its last import stamp (`.rockbox/rekordbox_sync.dat`)
2. It parses the PDB — artist, album, genre, and track metadata — and writes a temporary database file
3. It commits the entries into the Rockbox tagcache
4. It generates `.m3u8` playlist files in `/Playlists/Rekordbox/`
5. It writes `/PIONEER/database.ignore` so Rockbox's own file scanner doesn't double-index the Pioneer folder

This happens automatically in the background thread during boot. On a large collection (10,000+ tracks) it may take 20–40 seconds on first import. Subsequent boots are instant if nothing has changed.

---

## Browsing your library in Rockbox

After import, your Rekordbox tracks appear in the standard Rockbox database views:

- **Database → Artists** → all artists from your Rekordbox export
- **Database → Albums**
- **Database → Genres**
- **Database → All tracks**
- **Files → Playlists → Rekordbox →** your Rekordbox playlists as `.m3u8` files

If the database does not appear populated after first boot, trigger a manual update:

**Main Menu → Database → Update Now**

Or force a full rebuild:

**Main Menu → Database → Initialize Now**

---

## Using with Pioneer CDJs

CDJ models with USB storage support (CDJ-2000, CDJ-2000NXS, CDJ-2000NXS2, CDJ-3000, XDJ-1000, XDJ-RX series, and most post-2010 Pioneer/Denon media players) read directly from the `/PIONEER/rekordbox/export.pdb` database — the same file Rockbox imports from. No conflict.

**Workflow:**

1. Export from Rekordbox on your laptop to the iPod (USB)
2. Unplug from laptop — iPod re-imports on next boot automatically
3. At the venue: plug iPod into CDJ USB port — CDJ reads the Rekordbox database natively
4. Between sets: use the iPod standalone with Rockbox

**Known limitations:**
- The iPod must be formatted FAT32 — some CDJ firmware versions have issues with drives > 1TB or non-standard sector sizes. Standard iPod 5G/5.5G drives (80GB, 512-byte sectors) work reliably.
- If your CDJ requires the iPod to be in "disk mode" rather than standard USB MSC, enable it: hold **Menu + Select** on boot until the disk icon appears, then plug in.
- Rekordbox analysis data (waveforms, cue points, hot cues) is stored in the PDB and read by CDJs directly — Rockbox ignores this data but does not corrupt it.

---

## Re-syncing

Whenever you re-export from Rekordbox (adding tracks, editing cue points, updating playlists):

1. Connect iPod to laptop — Rockbox enters USB mode
2. Run the Rekordbox export as usual
3. Eject safely — on reconnect for normal use, Rockbox detects the updated PDB and re-imports

The re-import is incremental at the file level: tracks whose file mtime hasn't changed are skipped. Only new or modified entries are re-processed.

---

## Mixed library (Rekordbox + regular music)

You can have both Rekordbox-exported tracks and your own music on the same iPod:

- Put personal music anywhere **outside** `/PIONEER/` (e.g. `/Music/`)
- Rekordbox tracks live in `/PIONEER/Contents/`
- The `/PIONEER/database.ignore` file prevents the Rockbox scanner from double-indexing Pioneer tracks
- Enable **Main Menu → Database → Auto Update** so personal music outside `/PIONEER/` is scanned normally
- Both sets of tracks appear together in the Rockbox database

---

## Testing with the SDL Simulator

Before flashing your iPod, you can run and test the entire Rekordbox import pipeline locally using the Rockbox SDL simulator — no physical device needed.

### What the simulator is

Rockbox ships a first-party SDL2-based UI simulator that runs natively on Linux, macOS, and Windows. It emulates the Rockbox interface and treats a local `simdisk/` subdirectory as the iPod's drive. All of `pdb_parser.c`, `rekordbox_import.c`, and `tagcache.c` run inside it exactly as they would on real hardware.

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt install libsdl2-dev gcc make perl python3

# macOS
brew install sdl2
```

### Build the simulator

```bash
git clone <this repo>
cd rockbox
mkdir build-sim && cd build-sim
../tools/configure
# Select target: 22 (iPod Video)
# Select build type: [Ss] Simulator  (press A for advanced options first)
make -j$(nproc)
make install
```

This produces a `rockboxui` binary and a `simdisk/` directory in `build-sim/`.

### Set up a test library

Recreate the Pioneer folder structure inside `simdisk/`:

```bash
mkdir -p build-sim/simdisk/PIONEER/rekordbox
mkdir -p build-sim/simdisk/PIONEER/Contents/01

# Copy a real Rekordbox export
cp /path/to/your/export.pdb build-sim/simdisk/PIONEER/rekordbox/export.pdb

# Optionally add some audio files at the paths the PDB references
# (Rockbox will stat() each file; missing files are skipped gracefully)
cp /path/to/track.mp3 build-sim/simdisk/PIONEER/Contents/01/
```

### Run

```bash
cd build-sim
./rockboxui
```

On "boot", `tagcache_thread()` fires, detects the PDB, and runs the full import. Check the results:

```bash
# Sync stamp written after successful import
ls -la build-sim/simdisk/.rockbox/rekordbox_sync.dat

# Tagcache database files
ls build-sim/simdisk/.rockbox/database_*.tcd

# Generated playlists
find build-sim/simdisk/Playlists/Rekordbox/ -name '*.m3u8'

# Pioneer ignore marker
ls build-sim/simdisk/PIONEER/database.ignore
```

To force a re-import, delete the sync stamp:

```bash
rm build-sim/simdisk/.rockbox/rekordbox_sync.dat
# Then re-run ./rockboxui
```

### Simulator limitations

| Feature | Simulator | Real hardware |
|---------|-----------|---------------|
| PDB parsing | ✅ Full | ✅ Full |
| Tagcache import | ✅ Full | ✅ Full |
| Playlist generation | ✅ Full | ✅ Full |
| Audio playback | ❌ None | ✅ Full |
| USB connect/disconnect re-import | ❌ Not triggered | ✅ Automatic |
| FAT32 filesystem | ❌ Host FS only | ✅ FAT32 |
| Boot timing / performance | ❌ Host CPU speed | ✅ PP5022 speed |

The USB re-import path (`SYS_USB_CONNECTED` handler in `tagcache.c`) cannot be triggered in the simulator. Test it by deleting the sync stamp manually and rebooting the sim instead.

---


### Prerequisites

```bash
# Ubuntu/Debian
sudo apt install gcc gcc-arm-none-eabi make perl python3 zip

# macOS
brew install arm-none-eabi-gcc make
```

### Configure and build

```bash
git clone <this repo>
cd rockbox
mkdir build-ipodvideo && cd build-ipodvideo
../tools/configure
# Select: 22 (iPod Video)
# Build type: N (Normal)
make -j$(nproc)
make zip
```

The `make zip` step produces `rockbox.zip` — extract it and copy the `.rockbox` folder to your iPod.

### Verifying Rekordbox support is compiled in

After `configure`, check `build-ipodvideo/autoconf.h`:

```bash
grep HAVE_REKORDBOX autoconf.h
# Should output: #define HAVE_REKORDBOX
```

If it's missing, verify `firmware/export/config/ipodvideo.h` contains `#define HAVE_REKORDBOX`.

---

## Troubleshooting

**Database is empty after boot**
- Wait 30–60 seconds after first boot on a large collection, then go to **Database → Update Now**
- Check that `/PIONEER/rekordbox/export.pdb` exists on the drive
- Check `.rockbox/rekordbox_sync.dat` — if it exists and matches the PDB mtime, Rockbox considers it already imported. Delete it to force a re-import.

**Tracks show `<Untagged>` for some fields**
- The PDB entry for those tracks has empty metadata. Fix in Rekordbox and re-export.

**Playlists folder is empty**
- Check `/Playlists/Rekordbox/` on the drive — if the directory doesn't exist, the import may have failed partway through. Delete `.rockbox/rekordbox_sync.dat` and reboot.

**CDJ says "No USB device" or can't read library**
- Confirm the drive is FAT32, not exFAT or APFS
- Confirm `/PIONEER/rekordbox/export.pdb` is present (not just the audio files)
- Some older CDJ firmware versions require a `PIONEER` folder at the root — this is always created by a proper Rekordbox export

**Import takes very long on every boot**
- This should only happen on first import or after a re-export. If it repeats every boot, check that `.rockbox/rekordbox_sync.dat` is being written (the `/.rockbox` directory must be writable)

**PDB page size error in log**
- The internal page buffer is 4KB. Rekordbox 6.x/7.x uses 4KB pages, so this should not trigger. If it does (unusual custom export), file a bug with your `export.pdb` version details.

---

## Technical notes

- **No third-party scripts required.** The entire import pipeline runs inside Rockbox at boot time.
- **PDB format**: Rekordbox 6.x/7.x DeviceSQL format. Documented at [djl-analysis.deepsymmetry.org](https://djl-analysis.deepsymmetry.org/rekordbox-export-analysis/exports.html).
- **Tagcache integration**: entries are written as `temp_file_entry` records to `database_tmp.tcd`, then committed by the standard tagcache `commit()` path — identical to how Rockbox's own file scanner adds tracks.
- **Source files**: `apps/pdb_parser.{c,h}`, `apps/rekordbox_import.{c,h}`, integration in `apps/tagcache.c` behind `#ifdef HAVE_REKORDBOX`.
