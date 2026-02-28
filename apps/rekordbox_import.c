#ifdef HAVE_REKORDBOX

#include "config.h"
#include "rekordbox_import.h"
#include "pdb_parser.h"
#include "tagcache.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Rockbox kernel / filesystem headers */
#include "file.h"       /* open, close, read, write, lseek */
#include "dir.h"        /* mkdir */
#include "errno.h"      /* errno, EEXIST */
#include "logf.h"
#include "system.h"     /* MAX_PATH */

/* stat() for mtime retrieval */
#include <sys/stat.h>

/* -----------------------------------------------------------------------
 * Internal constants
 * ----------------------------------------------------------------------- */

/* Tagcache temp-file magic (must match TAGCACHE_MAGIC in tagcache.c). */
#define RB_TAGCACHE_MAGIC  0x54434810

/* Maximum byte length of a single tag string (from tagcache.h TAG_MAXLEN). */
#ifndef TAG_MAXLEN
#define TAG_MAXLEN  (MAX_PATH * 2)
#endif

/* Fallback string for empty/missing tags (from tagcache.h UNTAGGED). */
#ifndef UNTAGGED
#define UNTAGGED  "<Untagged>"
#endif

/* TAG_COUNT must come from tagcache.h via the enum tag_type. */
/* tag_artist=0 … tag_lastoffset=22, TAG_COUNT=23 */

/* -----------------------------------------------------------------------
 * Minimal local copies of the tagcache on-disk structures.
 *
 * These mirror the definitions in tagcache.c (which are not exported via
 * tagcache.h).  They must remain byte-for-byte identical.
 * ----------------------------------------------------------------------- */

struct rb_tagcache_header {
    int32_t magic;        /* RB_TAGCACHE_MAGIC */
    int32_t datasize;     /* Total string-data bytes written after header */
    int32_t entry_count;  /* Number of temp_file_entry records */
};

/* Full tag entry stored in database_tmp.tcd waiting for commit. */
struct rb_temp_file_entry {
    int32_t tag_offset[TAG_COUNT]; /* numeric: value stored directly;
                                      string:  byte offset into this
                                              entry's data blob */
    int16_t tag_length[TAG_COUNT]; /* 0 for numeric tags;
                                      strlen(str)+1 for string tags */
    int32_t flag;
    int32_t data_length;           /* total bytes of concatenated strings */
};

/* -----------------------------------------------------------------------
 * Helper: safe string copy that guarantees NUL termination and never
 * overflows dst.  Returns pointer to dst.
 * ----------------------------------------------------------------------- */
static char *safe_strcpy(char *dst, const char *src, size_t dst_size)
{
    if (!dst || dst_size == 0)
        return dst;
    if (!src || *src == '\0')
    {
        dst[0] = '\0';
        return dst;
    }
    size_t len = strlen(src);
    if (len >= dst_size)
        len = dst_size - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

/* -----------------------------------------------------------------------
 * Helper: return the effective string for a tag field.
 *
 * If s is NULL or empty, returns UNTAGGED.  The returned pointer is
 * either s itself or the UNTAGGED literal — never heap-allocated.
 * ----------------------------------------------------------------------- */
static const char *effective_tag(const char *s)
{
    if (!s || *s == '\0')
        return UNTAGGED;
    return s;
}

/* -----------------------------------------------------------------------
 * Build the on-device absolute path for a PDB track.
 *
 * PDB stores paths as "/Contents/..." relative to the Pioneer export root.
 * On a device that was exported to the root of the volume, the PIONEER
 * folder lives at /PIONEER, so the full path becomes
 *   /PIONEER/Contents/<filename>
 *
 * dst must be at least MAX_PATH bytes.
 * Returns true on success, false if the assembled path would be too long.
 * ----------------------------------------------------------------------- */
static bool build_track_path(char *dst, const char *pdb_file_path)
{
    int n = snprintf(dst, MAX_PATH, "%s%s", RB_PIONEER_ROOT, pdb_file_path);
    if (n < 0 || n >= MAX_PATH)
    {
        logf("rekordbox: path too long: %s%s", RB_PIONEER_ROOT, pdb_file_path);
        return false;
    }
    return true;
}

/* -----------------------------------------------------------------------
 * Sync stamp helpers.
 *
 * The stamp file contains exactly 4 bytes: a little-endian uint32_t
 * representing the Unix mtime of the last successfully imported PDB.
 * ----------------------------------------------------------------------- */

static uint32_t read_sync_stamp(void)
{
    int fd = open(RB_SYNC_STAMP, O_RDONLY);
    if (fd < 0)
        return 0;

    uint8_t buf[4];
    ssize_t n = read(fd, buf, 4);
    close(fd);
    if (n != 4)
        return 0;

    return (uint32_t)buf[0]
         | ((uint32_t)buf[1] << 8)
         | ((uint32_t)buf[2] << 16)
         | ((uint32_t)buf[3] << 24);
}

static void write_sync_stamp(uint32_t mtime)
{
    int fd = open(RB_SYNC_STAMP, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0)
    {
        logf("rekordbox: cannot write sync stamp");
        return;
    }
    uint8_t buf[4] = {
        (uint8_t)(mtime),
        (uint8_t)(mtime >> 8),
        (uint8_t)(mtime >> 16),
        (uint8_t)(mtime >> 24)
    };
    write(fd, buf, 4);
    close(fd);
}

/* -----------------------------------------------------------------------
 * Public: rekordbox_import_needed()
 * ----------------------------------------------------------------------- */

bool rekordbox_import_needed(void)
{
    struct stat st;
    if (stat(RB_PDB_PATH, &st) != 0)
        return false; /* PDB absent — nothing to do */

    uint32_t last = read_sync_stamp();
    return (uint32_t)st.st_mtime != last;
}

/* -----------------------------------------------------------------------
 * Write one temp_file_entry record to the open cachefd.
 *
 * The function mirrors the add_tagcache() write sequence in tagcache.c
 * exactly.  String tags are accumulated in tag_offset / tag_length, then
 * the struct header is written, followed by the NUL-terminated strings in
 * the mandatory order.
 *
 * total_data_size is updated by the caller; this function returns the
 * number of string-data bytes written for this entry (>= 0), or -1 on
 * write error.
 * ----------------------------------------------------------------------- */
static int write_track_entry(int fd, const struct pdb_track *t,
                             const char *abs_path, uint32_t mtime)
{
    struct rb_temp_file_entry entry;
    memset(&entry, 0, sizeof(entry));

    /* Effective strings — fall back to UNTAGGED when empty. */
    const char *s_path    = abs_path;
    const char *s_title   = effective_tag(t->title);
    const char *s_artist  = effective_tag(t->artist);
    const char *s_album   = effective_tag(t->album);
    const char *s_genre   = effective_tag(t->genre);
    const char *s_composer = effective_tag(t->composer);
    const char *s_comment  = effective_tag(t->comment);

    /* albumartist — PDB has no separate albumartist field; reuse artist. */
    const char *s_albumartist = effective_tag(t->artist);

    /* canonical artist: prefer artist, fall back to albumartist */
    bool has_artist = (t->artist[0] != '\0');
    const char *s_canonical = has_artist ? s_artist : s_albumartist;

    /* grouping — PDB has no grouping field; fall back to title. */
    const char *s_grouping = effective_tag(t->title);

    /*
     * Build tag_offset[] and tag_length[] for string tags.
     * tag_offset[tag] = cumulative byte offset in this entry's data blob.
     * tag_length[tag] = strlen(str) + 1 (includes NUL).
     *
     * Write order must match tagcache.c add_tagcache() exactly:
     *   filename, title, artist, album, genre, composer, comment,
     *   albumartist, virt_canonicalartist, grouping
     */
#define ADD_STRING_TAG(tag, str)                                \
    do {                                                        \
        int _len = (int)strlen(str) + 1;                        \
        if (_len > TAG_MAXLEN) _len = TAG_MAXLEN;               \
        entry.tag_length[tag] = (int16_t)_len;                  \
        entry.tag_offset[tag] = offset;                         \
        offset += _len;                                         \
    } while (0)

    int offset = 0;

    ADD_STRING_TAG(tag_filename,            s_path);
    ADD_STRING_TAG(tag_title,               s_title);
    ADD_STRING_TAG(tag_artist,              s_artist);
    ADD_STRING_TAG(tag_album,               s_album);
    ADD_STRING_TAG(tag_genre,               s_genre);
    ADD_STRING_TAG(tag_composer,            s_composer);
    ADD_STRING_TAG(tag_comment,             s_comment);
    ADD_STRING_TAG(tag_albumartist,         s_albumartist);
    ADD_STRING_TAG(tag_virt_canonicalartist, s_canonical);
    ADD_STRING_TAG(tag_grouping,            s_grouping);

#undef ADD_STRING_TAG

    entry.data_length = offset;

    /* Numeric tags — stored directly in tag_offset[]. */
    entry.tag_offset[tag_year]        = t->year;
    entry.tag_offset[tag_discnumber]  = t->disc_number;
    entry.tag_offset[tag_tracknumber] = t->track_number;
    entry.tag_offset[tag_length]      = (int32_t)t->duration * 1000; /* ms */
    entry.tag_offset[tag_bitrate]     = (int32_t)t->bitrate;
    entry.tag_offset[tag_mtime]       = (int32_t)mtime;
    entry.tag_offset[tag_rating]      = (int32_t)t->rating;
    entry.tag_offset[tag_playcount]   = 0;
    entry.tag_offset[tag_playtime]    = 0;
    entry.tag_offset[tag_lastplayed]  = 0;
    entry.tag_offset[tag_commitid]    = 0;
    entry.tag_offset[tag_lastelapsed] = 0;
    entry.tag_offset[tag_lastoffset]  = 0;

    /* Write header struct */
    if (write(fd, &entry, sizeof(entry)) != (ssize_t)sizeof(entry))
    {
        logf("rekordbox: write entry header failed");
        return -1;
    }

    /* Write string data in the mandatory order */
#define WRITE_STR(str)                                          \
    do {                                                        \
        int _len = (int)strlen(str) + 1;                        \
        if (_len > TAG_MAXLEN) _len = TAG_MAXLEN;               \
        if (write(fd, str, _len) != _len)                       \
        {                                                        \
            logf("rekordbox: write string failed");             \
            return -1;                                          \
        }                                                       \
    } while (0)

    WRITE_STR(s_path);
    WRITE_STR(s_title);
    WRITE_STR(s_artist);
    WRITE_STR(s_album);
    WRITE_STR(s_genre);
    WRITE_STR(s_composer);
    WRITE_STR(s_comment);
    WRITE_STR(s_albumartist);
    WRITE_STR(s_canonical);
    WRITE_STR(s_grouping);

#undef WRITE_STR

    return entry.data_length;
}

/* -----------------------------------------------------------------------
 * Write database_tmp.tcd from the parsed PDB track array.
 *
 * Returns number of entries written, or -1 on error.
 * ----------------------------------------------------------------------- */
static int write_tagcache_tmp(const char *db_path,
                              const struct pdb_track *tracks,
                              int track_count)
{
    char tmp_path[MAX_PATH];
    int n = snprintf(tmp_path, sizeof(tmp_path), "%s/%s",
                     db_path, "database_tmp.tcd");
    if (n < 0 || n >= (int)sizeof(tmp_path))
    {
        logf("rekordbox: db_path too long");
        return -1;
    }

    /* If the file already exists (e.g. a previous interrupted build),
     * skip — the existing temp file will be committed by tagcache_thread. */
    struct stat st_check;
    if (stat(tmp_path, &st_check) == 0)
    {
        logf("rekordbox: database_tmp.tcd already present, skipping write");
        return 0;
    }

    int fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0)
    {
        logf("rekordbox: cannot open database_tmp.tcd for writing");
        return -1;
    }

    /* Write placeholder header — we'll fill in counts at the end. */
    struct rb_tagcache_header hdr;
    memset(&hdr, 0, sizeof(hdr));
    if (write(fd, &hdr, sizeof(hdr)) != (ssize_t)sizeof(hdr))
    {
        logf("rekordbox: header write failed");
        close(fd);
        return -1;
    }

    int32_t total_data   = 0;
    int32_t entry_count  = 0;
    char abs_path[MAX_PATH];

    for (int i = 0; i < track_count; i++)
    {
        const struct pdb_track *t = &tracks[i];

        if (!build_track_path(abs_path, t->file_path))
            continue;

        /* Verify the file actually exists on disk before adding. */
        struct stat st;
        if (stat(abs_path, &st) != 0)
        {
            logf("rekordbox: track file not found: %s", abs_path);
            continue;
        }

        /* Use the file's mtime for tagcache duplicate-detection.
         * If mtime is 0 (unusual on FAT), use 1 as a non-zero sentinel
         * so the entry is not treated as 'unknown'. */
        uint32_t mtime = (st.st_mtime != 0) ? (uint32_t)st.st_mtime : 1;

        int data_written = write_track_entry(fd, t, abs_path, mtime);
        if (data_written < 0)
        {
            close(fd);
            return -1;
        }

        total_data  += data_written;
        entry_count++;
    }

    /* Go back and write the real header. */
    hdr.magic       = RB_TAGCACHE_MAGIC;
    hdr.datasize    = total_data;
    hdr.entry_count = entry_count;

    if (lseek(fd, 0, SEEK_SET) < 0
        || write(fd, &hdr, sizeof(hdr)) != (ssize_t)sizeof(hdr))
    {
        logf("rekordbox: header rewrite failed");
        close(fd);
        return -1;
    }

    close(fd);
    logf("rekordbox: wrote %d track entries to database_tmp.tcd", entry_count);
    return entry_count;
}

/* -----------------------------------------------------------------------
 * M3U8 playlist generator.
 *
 * For each leaf playlist in the PDB playlist_tree, creates a .m3u8 file
 * under /Playlists/Rekordbox/<parent_folder_name>/<playlist_name>.m3u8
 * (or directly in /Playlists/Rekordbox/ for top-level playlists).
 *
 * Entries are sorted by their entry_index field (simple insertion sort
 * on the small subset that belongs to each playlist — typical playlists
 * have dozens to a few hundred tracks).
 * ----------------------------------------------------------------------- */

/* Find a playlist_tree entry by its ID.  Linear scan is fine given the
 * small sizes involved (≤ PDB_MAX_PLAYLISTS = 2000). */
static const struct pdb_playlist_tree_entry *
find_tree_entry(const struct pdb_playlist_tree_entry *tree, int count,
                uint32_t id)
{
    for (int i = 0; i < count; i++)
        if (tree[i].id == id)
            return &tree[i];
    return NULL;
}

/* Build the filesystem path for a playlist file.
 * We support one level of folder nesting (parent → playlist).
 * Deeper nesting is flattened to the parent level.
 *
 * Returns true on success. */
static bool build_playlist_path(
    char *dst, size_t dst_size,
    const struct pdb_playlist_tree_entry *pl,
    const struct pdb_playlist_tree_entry *tree, int tree_count)
{
    const char *pl_name = pl->name[0] ? pl->name : "Unnamed";

    /* Find parent folder (if any) */
    if (pl->parent_id != 0)
    {
        const struct pdb_playlist_tree_entry *parent =
            find_tree_entry(tree, tree_count, pl->parent_id);

        if (parent && parent->is_folder && parent->name[0])
        {
            int n = snprintf(dst, dst_size, "%s/%s/%s.m3u8",
                             RB_PLAYLIST_DIR, parent->name, pl_name);
            if (n > 0 && n < (int)dst_size)
                return true;
        }
    }

    /* Top-level or parent not found */
    int n = snprintf(dst, dst_size, "%s/%s.m3u8", RB_PLAYLIST_DIR, pl_name);
    return (n > 0 && n < (int)dst_size);
}

/* Ensure a directory exists, creating it (and its parent under
 * RB_PLAYLIST_DIR) if needed.  We only handle one level of depth beyond
 * RB_PLAYLIST_DIR. */
static void ensure_dir(const char *dir_path)
{
    if (mkdir(dir_path) < 0 && errno != EEXIST)
        logf("rekordbox: mkdir %s failed", dir_path);
}

/* Compare two playlist entries by entry_index for sorting. */
static int cmp_playlist_entry(const void *a, const void *b)
{
    const struct pdb_playlist_entry *ea = (const struct pdb_playlist_entry *)a;
    const struct pdb_playlist_entry *eb = (const struct pdb_playlist_entry *)b;
    if (ea->entry_index < eb->entry_index) return -1;
    if (ea->entry_index > eb->entry_index) return  1;
    return 0;
}

/* Write a single .m3u8 file for the playlist whose ID is pl_id.
 *
 * track_lookup : all parsed pdb_track records (for path resolution).
 * entries      : all pdb_playlist_entry records across all playlists.
 *
 * A temporary stack-allocated sort buffer is used; for very large
 * playlists (> PDB_MAX_PLAYLIST_ENTRIES) we truncate silently.
 */
#define PL_SORT_BUF_MAX  PDB_MAX_PLAYLIST_ENTRIES

static void write_playlist_file(
    const char *path,
    uint32_t pl_id,
    const struct pdb_track *tracks, int track_count,
    const struct pdb_playlist_entry *entries, int entry_count)
{
    /* Collect entries for this playlist into a local buffer. */
    static struct pdb_playlist_entry sort_buf[PL_SORT_BUF_MAX];
    int n_pl = 0;

    for (int i = 0; i < entry_count && n_pl < PL_SORT_BUF_MAX; i++)
    {
        if (entries[i].playlist_id == pl_id)
            sort_buf[n_pl++] = entries[i];
    }

    if (n_pl == 0)
        return; /* empty playlist — skip */

    /* Sort by entry_index (ascending). */
    /* Simple insertion sort — n_pl is typically small. */
    for (int i = 1; i < n_pl; i++)
    {
        struct pdb_playlist_entry key = sort_buf[i];
        int j = i - 1;
        while (j >= 0 && sort_buf[j].entry_index > key.entry_index)
        {
            sort_buf[j + 1] = sort_buf[j];
            j--;
        }
        sort_buf[j + 1] = key;
    }

    (void)cmp_playlist_entry; /* suppress unused-function warning */

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0)
    {
        logf("rekordbox: cannot create playlist: %s", path);
        return;
    }

    static const char header[] = "#EXTM3U\n";
    write(fd, header, sizeof(header) - 1);

    for (int i = 0; i < n_pl; i++)
    {
        uint32_t tid = sort_buf[i].track_id;
        const struct pdb_track *t = NULL;

        /* Linear search — acceptable for typical collection sizes. */
        for (int j = 0; j < track_count; j++)
        {
            if (tracks[j].id == tid)
            {
                t = &tracks[j];
                break;
            }
        }

        if (!t)
            continue;

        char abs_path[MAX_PATH];
        if (!build_track_path(abs_path, t->file_path))
            continue;

        write(fd, abs_path, strlen(abs_path));
        write(fd, "\n", 1);
    }

    close(fd);
}

/* Generate all .m3u8 playlist files from the parsed PDB data. */
static void generate_playlists(
    const struct pdb_playlist_tree_entry *tree, int tree_count,
    const struct pdb_playlist_entry *entries, int entry_count,
    const struct pdb_track *tracks, int track_count)
{
    /* Ensure base playlist directory exists. */
    ensure_dir(RB_PLAYLIST_DIR);

    for (int i = 0; i < tree_count; i++)
    {
        const struct pdb_playlist_tree_entry *pl = &tree[i];

        /* Skip folders — only write files for actual playlists. */
        if (pl->is_folder)
        {
            /* Pre-create the subdirectory so leaf playlists can be
             * created inside it. */
            char subdir[MAX_PATH];
            int n = snprintf(subdir, sizeof(subdir), "%s/%s",
                             RB_PLAYLIST_DIR, pl->name[0] ? pl->name : "Folder");
            if (n > 0 && n < (int)sizeof(subdir))
                ensure_dir(subdir);
            continue;
        }

        char pl_path[MAX_PATH];
        if (!build_playlist_path(pl_path, sizeof(pl_path), pl, tree, tree_count))
        {
            logf("rekordbox: playlist path too long for '%s'", pl->name);
            continue;
        }

        write_playlist_file(pl_path, pl->id,
                            tracks, track_count,
                            entries, entry_count);
    }

    logf("rekordbox: playlist generation complete");
}

/* -----------------------------------------------------------------------
 * Write the database.ignore marker file so that tagcache's own file-system
 * scanner does not double-index the PIONEER folder contents.
 * ----------------------------------------------------------------------- */
static void write_ignore_file(void)
{
    int fd = open(RB_IGNORE_FILE, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0)
    {
        logf("rekordbox: cannot create %s", RB_IGNORE_FILE);
        return;
    }
    close(fd);
}

/* -----------------------------------------------------------------------
 * Public: rekordbox_run_import()
 * ----------------------------------------------------------------------- */

int rekordbox_run_import(uint8_t *page_buf, uint32_t page_buf_size,
                         const char *db_path)
{
    /* ------------------------------------------------------------------ */
    /* 1. Open export.pdb                                                  */
    /* ------------------------------------------------------------------ */
    int pdb_fd = open(RB_PDB_PATH, O_RDONLY);
    if (pdb_fd < 0)
    {
        logf("rekordbox: cannot open %s", RB_PDB_PATH);
        return -1;
    }

    /* Record the PDB mtime before we start so we can stamp it later. */
    struct stat pdb_stat;
    uint32_t pdb_mtime = 0;
    if (stat(RB_PDB_PATH, &pdb_stat) == 0)
        pdb_mtime = (uint32_t)pdb_stat.st_mtime;

    /* ------------------------------------------------------------------ */
    /* 2. Initialise PDB context                                           */
    /* ------------------------------------------------------------------ */
    struct pdb_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.fd           = pdb_fd;
    ctx.page_buf      = page_buf;
    ctx.page_buf_size = page_buf_size;

    if (!pdb_open(&ctx))
    {
        logf("rekordbox: pdb_open failed");
        close(pdb_fd);
        return -1;
    }

    /* Check that the provided page buffer is large enough. */
    if (ctx.page_size > page_buf_size)
    {
        logf("rekordbox: page_buf too small (%u needed, %u provided)",
             ctx.page_size, page_buf_size);
        pdb_close(&ctx);
        close(pdb_fd);
        return -1;
    }

    /* ------------------------------------------------------------------ */
    /* 3. Build artist/album/genre lookup tables                           */
    /* ------------------------------------------------------------------ */
    if (pdb_build_lookup_tables(&ctx) < 0)
    {
        logf("rekordbox: pdb_build_lookup_tables failed");
        pdb_close(&ctx);
        close(pdb_fd);
        return -1;
    }

    /* ------------------------------------------------------------------ */
    /* 4. Parse tracks                                                     */
    /* ------------------------------------------------------------------ */

    /* Allocate track buffer on the heap.  On Rockbox embedded targets the
     * audio buffer would normally be used, but here we use malloc-style
     * allocation via the buffer_alloc system.  Since rekordbox_run_import
     * is called from tagcache_thread before any audio playback begins, a
     * heap allocation is safe and simpler to manage.
     *
     * At ~800 bytes per pdb_track and a limit of 50 000 tracks that is
     * ~40 MB — clearly too large for small targets.  We therefore use a
     * practical cap and log a warning if we hit it.
     *
     * TODO: on memory-constrained targets, process tracks in chunks and
     * write partial temp files, then concatenate.  For now the cap is the
     * pragmatic choice.
     */
#define RB_IMPORT_MAX_TRACKS  20000  /* hard limit for this implementation */

    struct pdb_track *tracks =
        (struct pdb_track *)malloc(RB_IMPORT_MAX_TRACKS * sizeof(struct pdb_track));
    if (!tracks)
    {
        logf("rekordbox: malloc failed for track buffer");
        pdb_close(&ctx);
        close(pdb_fd);
        return -1;
    }

    int track_count = pdb_parse_tracks(&ctx, tracks, RB_IMPORT_MAX_TRACKS);
    if (track_count < 0)
    {
        logf("rekordbox: pdb_parse_tracks failed");
        free(tracks);
        pdb_close(&ctx);
        close(pdb_fd);
        return -1;
    }

    logf("rekordbox: parsed %d tracks", track_count);

    /* ------------------------------------------------------------------ */
    /* 5. Write database_tmp.tcd                                           */
    /* ------------------------------------------------------------------ */
    int written = write_tagcache_tmp(db_path, tracks, track_count);
    if (written < 0)
    {
        logf("rekordbox: write_tagcache_tmp failed");
        free(tracks);
        pdb_close(&ctx);
        close(pdb_fd);
        return -1;
    }

    /* ------------------------------------------------------------------ */
    /* 6. Parse playlists                                                  */
    /* ------------------------------------------------------------------ */

    /* Allocate playlist buffers on the heap as well. */
    struct pdb_playlist_tree_entry *pl_tree =
        (struct pdb_playlist_tree_entry *)malloc(
            PDB_MAX_PLAYLISTS * sizeof(struct pdb_playlist_tree_entry));
    struct pdb_playlist_entry *pl_entries =
        (struct pdb_playlist_entry *)malloc(
            PDB_MAX_PLAYLIST_ENTRIES * sizeof(struct pdb_playlist_entry));

    int pl_total = 0;
    if (pl_tree && pl_entries)
    {
        ctx.playlist_tree        = pl_tree;
        ctx.playlist_entries      = pl_entries;
        ctx.playlist_tree_count   = 0;
        ctx.playlist_entry_count  = 0;

        pl_total = pdb_parse_playlists(&ctx,
                                       pl_tree,   PDB_MAX_PLAYLISTS,
                                       pl_entries, PDB_MAX_PLAYLIST_ENTRIES);
        if (pl_total < 0)
        {
            logf("rekordbox: pdb_parse_playlists failed (non-fatal)");
            pl_total = 0;
        }
        else
        {
            logf("rekordbox: parsed %d playlist tree + %d entries",
                 ctx.playlist_tree_count, ctx.playlist_entry_count);
        }
    }
    else
    {
        logf("rekordbox: playlist buffer allocation failed (skipping playlists)");
    }

    /* ------------------------------------------------------------------ */
    /* 7. Generate .m3u8 files                                             */
    /* ------------------------------------------------------------------ */
    if (pl_total > 0 && pl_tree && pl_entries)
    {
        generate_playlists(pl_tree, ctx.playlist_tree_count,
                           pl_entries, ctx.playlist_entry_count,
                           tracks, track_count);
    }

    /* ------------------------------------------------------------------ */
    /* 8. Cleanup                                                          */
    /* ------------------------------------------------------------------ */
    if (pl_tree)    free(pl_tree);
    if (pl_entries) free(pl_entries);
    free(tracks);
    pdb_close(&ctx);
    close(pdb_fd);

    /* ------------------------------------------------------------------ */
    /* 9. Write database.ignore and update sync stamp                      */
    /* ------------------------------------------------------------------ */
    write_ignore_file();

    if (pdb_mtime != 0)
        write_sync_stamp(pdb_mtime);

    logf("rekordbox: import complete — %d entries written", written);
    return written;
}

#endif /* HAVE_REKORDBOX */
