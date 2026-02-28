#ifndef _REKORDBOX_IMPORT_H
#define _REKORDBOX_IMPORT_H

#ifdef HAVE_REKORDBOX

#include <stdbool.h>

/* Absolute path of the Pioneer export database on the device. */
#define RB_PDB_PATH        "/PIONEER/rekordbox/export.pdb"

/* Root directory for Pioneer content on the device.
 * PDB stores paths as "/Contents/...", which map to this prefix. */
#define RB_PIONEER_ROOT    "/PIONEER"

/* File placed in the PIONEER directory to prevent tagcache's own
 * file-system scanner from indexing those files a second time. */
#define RB_IGNORE_FILE     "/PIONEER/database.ignore"

/* Output directory for generated Rekordbox playlist files. */
#define RB_PLAYLIST_DIR    "/Playlists/Rekordbox"

/* Rockbox database directory (matches ROCKBOX_DIR / tagcache_db_path). */
#define RB_DB_DIR          "/.rockbox"

/* Stamp file that records the mtime of the last successfully imported PDB.
 * Content is a single 32-bit little-endian Unix timestamp. */
#define RB_SYNC_STAMP      "/.rockbox/rekordbox_sync.dat"

/*
 * rekordbox_import_needed() - quick check before allocating anything.
 *
 * Returns true if /PIONEER/rekordbox/export.pdb exists AND its mtime
 * differs from the last-synced mtime stored in RB_SYNC_STAMP.
 * Returns false if the file is absent or has not changed.
 */
bool rekordbox_import_needed(void);

/*
 * rekordbox_run_import() - full import pipeline.
 *
 * Caller must ensure database_tmp.tcd does NOT already exist (i.e. call
 * this before do_tagcache_build / tagcache_build, or only when the temp
 * file is absent).
 *
 * Steps performed:
 *   1. Open /PIONEER/rekordbox/export.pdb
 *   2. Parse artist/album/genre lookup tables
 *   3. Parse track records
 *   4. Write database_tmp.tcd in temp_file_entry format
 *   5. Parse playlists and write .m3u8 files to /Playlists/Rekordbox/
 *   6. Write /PIONEER/database.ignore
 *   7. Update RB_SYNC_STAMP with the PDB's current mtime
 *
 * page_buf      : scratch buffer for PDB page reads (must be ≥ 4096 bytes;
 *                 pass a larger buffer if typical PDB page size is bigger —
 *                 pdb_open() will report the required size and the function
 *                 will bail if the buffer is too small).
 * page_buf_size : size of page_buf in bytes.
 * db_path       : path to the Rockbox database directory (tc_stat.db_path).
 *
 * Returns number of tracks written to database_tmp.tcd, or -1 on error.
 */
int rekordbox_run_import(uint8_t *page_buf, uint32_t page_buf_size,
                         const char *db_path);

#endif /* HAVE_REKORDBOX */
#endif /* _REKORDBOX_IMPORT_H */
