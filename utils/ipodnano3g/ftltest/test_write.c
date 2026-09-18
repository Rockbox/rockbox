/* Write-path test for ftl-nano3g.c: write, sync, remount, read back - all
 * against a copy-on-write mapping of the 4GB dump, never a device.
 *
 * Beyond "does the data come back", this checks two things that matter for
 * a real iPod: that we never break a NAND rule (no reprogram, no page
 * skipped inside a block), and that the image we leave behind still
 * satisfies the invariants Apple's own _LoadFTLCxt and _FTLRestore check -
 * so its firmware can still mount what we wrote. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nand-target.h"
#include "ftl-target.h"
#include "mock_nand.h"

#define BANKS   4
#define BLOCKS  4096
#define PPB     128
#define PLANES  2
#define SBPAGES (PPB * PLANES * BANKS)          /* 1024 */
#define NSB     ((BLOCKS - PLANES * 89) / PLANES) /* 1959 superblocks */
#define NPAGES  ((size_t)BANKS * BLOCKS * PPB)
#define POOL    20

static int fails;

/* src for the scattered-write test */
struct scat_ctx { uint32_t lpn[2]; uint8_t *p[2]; };

static const void *scat_src(uint32_t lpn, void *ctx)
{
    struct scat_ctx *s = ctx;
    int i;

    for (i = 0; i < 2; i++)
        if (s->lpn[i] == lpn)
            return s->p[i];
    return NULL;
}

static void check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        fails++;
}

static uint32_t meta_word(size_t g, int w)
{
    uint32_t v;
    memcpy(&v, mock_meta + g * 16 + 4 * w, 4);
    return v;
}

static int meta_type_g(size_t g)
{
    return mock_meta[g * 16 + 9];
}

/* (superblock, position within it) -> physical page, through the FTL's own
 * mapping so that VFL block remapping is accounted for */
static size_t g_of(uint32_t sb, uint32_t v)
{
    uint32_t bank, page;

    if (ftl_sb_locate(sb, v, &bank, &page))
        return (size_t)-1;
    return (size_t)bank * BLOCKS * PPB + page;
}

/* 0xff - "erased" - for a page that cannot be located, so a failed mount
 * reports failed checks instead of reading out of bounds */
static int type_at(uint32_t sb, uint32_t v)
{
    size_t g = g_of(sb, v);

    return g == (size_t)-1 ? 0xff : meta_type_g(g);
}

/* Would Apple's firmware mount this image? Re-derives everything from the
 * spare metadata, using none of the FTL's own state. */
static void check_apple_invariants(uint32_t usersb)
{
    static uint8_t state[NSB];
    uint32_t sb, v, ctrl[3], nctrl = 0, ndata = 0, nfree = 0, nlog = 0;
    uint32_t best = NSB, bestv = 0, bestusn = 0xffffffff;
    int last_is_cxt;

    for (sb = 0; sb < NSB; sb++)
    {
        int tlast = type_at(sb, SBPAGES - 1);
        int tfirst = type_at(sb, 0);

        if (tlast == 0x41)
            state[sb] = 0x41, ndata++;
        else if (tlast >= 0x43 && tlast <= 0x4f)
        {
            state[sb] = 0x42;
            if (nctrl < 3)
                ctrl[nctrl] = sb;
            nctrl++;
        }
        else if (tlast == 0xff && tfirst == 0xff)
            state[sb] = 0x48, nfree++;
        else if (tlast == 0xff && tfirst == 0x40)
            state[sb] = 0x40, nlog++;
        else if (tlast == 0xff && tfirst >= 0x43 && tfirst <= 0x4f)
        {
            state[sb] = 0x42;
            if (nctrl < 3)
                ctrl[nctrl] = sb;
            nctrl++;
        }
        else
            state[sb] = 0;
    }

    check("every superblock classifies as data, control, log or free",
          ndata + nctrl + nfree + nlog == NSB);
    check("one data superblock per logical block", ndata == usersb);
    check("exactly three control superblocks", nctrl == 3);
    check("free blocks + logs == 20 (_FTLRestore's invariant)",
          nfree + nlog == POOL);

    /* _LoadFTLCxt: of the three control blocks, the one whose page 0 has
     * the lowest usn is chosen; scanning it backward past erased pages,
     * the first readable page must be the context (0x43). */
    for (v = 0; v < nctrl && v < 3; v++)
    {
        size_t cg = g_of(ctrl[v], 0);
        uint32_t usn = cg == (size_t)-1 ? 0xffffffff : meta_word(cg, 0);
        int t = type_at(ctrl[v], 0);
        if (t >= 0x43 && t <= 0x4f && usn <= bestusn)
        {
            bestusn = usn;
            best = ctrl[v];
        }
    }
    check("a control block is selectable by page 0", best != NSB);
    if (best == NSB)
        return;
    last_is_cxt = 0;
    for (v = SBPAGES; v-- > 0; )
    {
        int t = type_at(best, v);
        if (t == 0xff)
            continue;
        last_is_cxt = (t == 0x43);
        bestv = v;
        break;
    }
    check("the newest control block's last written page is the context",
          last_is_cxt);
    check("the context is not the block's very last page (room to grow)",
          bestv < SBPAGES - 1);
    (void)state;
}

/* What Apple's _LoadFTLCxt finds: in the control block whose page 0 has the
 * lowest usn, the last written page. A context (0x43) there is trusted; a
 * 0x4f mark after it makes Apple run _FTLRestore instead. Returns the page
 * type and sets *usn to that page's usn. */
static int apple_ctrl_tail(uint32_t *usn)
{
    uint32_t sb, v, best = NSB, bestusn = 0xffffffff;

    for (sb = 0; sb < NSB; sb++)
    {
        int t = type_at(sb, 0);
        size_t cg = g_of(sb, 0);

        if (t < 0x43 || t > 0x4f || cg == (size_t)-1)
            continue;
        if (meta_word(cg, 0) <= bestusn)
        {
            bestusn = meta_word(cg, 0);
            best = sb;
        }
    }
    *usn = 0;
    if (best == NSB)
        return -1;
    for (v = SBPAGES; v-- > 0; )
    {
        int t = type_at(best, v);

        if (t == 0xff)
            continue;
        *usn = meta_word(g_of(best, v), 0);
        return t;
    }
    return -1;
}

/* src for the dirty-mark test: page idx of every block in [first, first+n) */
struct every_ctx { uint32_t first, nblk, idx; uint8_t *page; };

static const void *every_src(uint32_t lpn, void *ctx)
{
    struct every_ctx *e = ctx;
    uint32_t b = lpn / SBPAGES;

    if (lpn % SBPAGES != e->idx || b < e->first || b >= e->first + e->nblk)
        return NULL;
    return e->page;
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : "../n3g-dump";
    static uint8_t pattern[8 * NAND_PAGE_SIZE];
    static uint8_t saved[8 * NAND_PAGE_SIZE];
    static uint8_t before[8 * NAND_PAGE_SIZE];
    static uint8_t buf[8 * NAND_PAGE_SIZE];
    uint32_t nsec, usersb, base, n = 6, i;
    unsigned long sample_bad = 0;
    uint32_t *sample_lpn;
    uint8_t *sample_first;
    int rc;

    mock_open(dir);
    rc = ftl_init();
    check("mounts", rc == 0);
    if (rc)
        return 1;
    check("write state loaded", ftl_hook_writeable());
    nsec = ftl_num_sectors();
    usersb = nsec / SBPAGES;
    check("capacity is a whole number of superblocks", nsec % SBPAGES == 0);

    /* A sample of the rest of the medium, to prove writing moved nothing
     * it should not have */
    sample_lpn = malloc(2048 * sizeof(uint32_t));
    sample_first = malloc(2048);
    for (i = 0; i < 2048; i++)
    {
        sample_lpn[i] = (uint32_t)(((uint64_t)i * 967) % nsec);
        if (ftl_read(sample_lpn[i], 1, buf))
            sample_bad++;
        sample_first[i] = buf[0];
    }
    check("sampled the medium before writing", sample_bad == 0);

    /* Write across a superblock boundary */
    base = 5 * SBPAGES - 3;
    check("saved the region being overwritten", ftl_read(base, n, saved) == 0);
    check("saved the region just before it",
          ftl_read(base - 4, 4, before) == 0);
    for (i = 0; i < n * NAND_PAGE_SIZE; i++)
        pattern[i] = (uint8_t)(i * 31 + 7);

    mock_reset_stats();
    check("write returns 0", ftl_write(base, n, pattern) == 0);
    check("reads back as written before any sync",
          ftl_read(base, n, buf) == 0
          && !memcmp(buf, pattern, n * NAND_PAGE_SIZE));
    check("the sectors just before are untouched",
          ftl_read(base - 4, 4, buf) == 0 && !memcmp(buf, before, 4 * NAND_PAGE_SIZE));
    check("sync returns 0", ftl_sync() == 0);
    check("no page was reprogrammed", mock_reprogram == 0);
    check("no page was skipped within a block", mock_nonsequential == 0);
    printf("     %lu reads, %lu writes, %lu erases for 6 sectors + sync\n",
           mock_reads, mock_writes, mock_erases);

    /* Remount from what was actually written */
    rc = ftl_init();
    check("remounts after the commit", rc == 0);
    check("still writeable after remount", ftl_hook_writeable());
    check("capacity unchanged", ftl_num_sectors() == nsec);
    check("the written sectors survive the remount",
          ftl_read(base, n, buf) == 0
          && !memcmp(buf, pattern, n * NAND_PAGE_SIZE));
    check("the sectors just before survive too",
          ftl_read(base - 4, 4, buf) == 0
          && !memcmp(buf, before, 4 * NAND_PAGE_SIZE));

    /* Overwrite the same region again, to exercise the committed path */
    mock_reset_stats();
    for (i = 0; i < n * NAND_PAGE_SIZE; i++)
        pattern[i] = (uint8_t)(i * 17 + 3);
    check("second write returns 0", ftl_write(base, n, pattern) == 0);
    check("second sync returns 0", ftl_sync() == 0);
    check("second write raises no NAND violations",
          mock_reprogram == 0 && mock_nonsequential == 0);
    rc = ftl_init();
    check("remounts again", rc == 0);
    check("the second write survives",
          ftl_read(base, n, buf) == 0
          && !memcmp(buf, pattern, n * NAND_PAGE_SIZE));

    /* Restoring the original content must give back exactly what was there */
    check("restoring the original content works",
          ftl_write(base, n, saved) == 0 && ftl_sync() == 0);
    check("the region reads as it originally did",
          ftl_read(base, n, buf) == 0 && !memcmp(buf, saved, n * NAND_PAGE_SIZE));

    /* Nothing else moved */
    sample_bad = 0;
    for (i = 0; i < 2048; i++)
    {
        if (sample_lpn[i] >= base && sample_lpn[i] < base + n)
            continue;
        if (ftl_read(sample_lpn[i], 1, buf) || buf[0] != sample_first[i])
            sample_bad++;
    }
    check("the rest of the medium is unchanged", sample_bad == 0);

    /* A write spanning several superblocks */
    {
        static uint8_t big[48 * NAND_PAGE_SIZE];
        static uint8_t bigback[48 * NAND_PAGE_SIZE];
        uint32_t bbase = 40 * SBPAGES - 20, bn = 48;   /* spans 3 superblocks */

        for (i = 0; i < bn * NAND_PAGE_SIZE; i++)
            big[i] = (uint8_t)(i * 5 + 11);
        mock_reset_stats();
        check("a write spanning three superblocks returns 0",
              ftl_write(bbase, bn, big) == 0);
        check("it raises no NAND violations",
              mock_reprogram == 0 && mock_nonsequential == 0);
        check("sync after it returns 0", ftl_sync() == 0);
        check("it survives a remount",
              ftl_init() == 0 && ftl_read(bbase, bn, bigback) == 0
              && !memcmp(bigback, big, bn * NAND_PAGE_SIZE));
    }

    /* Scattered sectors in two logical blocks five apart. Before log blocks
     * this cost two whole block copies; now it should cost a couple of page
     * appends, and only the eventual sync pays for folding them back. */
    {
        static uint8_t sc[2][NAND_PAGE_SIZE];
        uint32_t lo = 60 * SBPAGES + 5, hi = 65 * SBPAGES + 7;

        for (i = 0; i < NAND_PAGE_SIZE; i++)
        {
            sc[0][i] = (uint8_t)(i * 3 + 1);
            sc[1][i] = (uint8_t)(i * 9 + 2);
        }
        struct scat_ctx sctx = { { lo, hi }, { sc[0], sc[1] } };
        mock_reset_stats();
        check("a scattered write returns 0",
              ftl_hook_write_merge(lo, hi, scat_src, &sctx) == 0);
        printf("     scattered write of 2 sectors: %lu pages written\n",
               mock_writes);
        check("it appended pages instead of copying two blocks",
              mock_writes < 16);
        check("it raises no NAND violations",
              mock_reprogram == 0 && mock_nonsequential == 0);
        check("both scattered sectors read back",
              ftl_read(lo, 1, buf) == 0 && !memcmp(buf, sc[0], NAND_PAGE_SIZE)
              && ftl_read(hi, 1, buf) == 0
              && !memcmp(buf, sc[1], NAND_PAGE_SIZE));
        check("they survive a sync and remount",
              ftl_sync() == 0 && ftl_init() == 0
              && ftl_read(lo, 1, buf) == 0 && !memcmp(buf, sc[0], NAND_PAGE_SIZE)
              && ftl_read(hi, 1, buf) == 0
              && !memcmp(buf, sc[1], NAND_PAGE_SIZE));
    }

    /* An unclean shutdown: write, then remount without syncing. The merged
     * blocks are not in the committed map, so the mount has to find them by
     * scanning - exactly the case Apple's _FTLRestore exists for. */
    for (i = 0; i < n * NAND_PAGE_SIZE; i++)
        pattern[i] = (uint8_t)(i * 13 + 5);
    check("write before the simulated power loss", ftl_write(base, n, pattern) == 0);
    rc = ftl_init();
    check("mounts after an unclean shutdown", rc == 0);
    check("still writeable after an unclean shutdown", ftl_hook_writeable());
    check("the unsynced write is found by the log scan",
          ftl_read(base, n, buf) == 0
          && !memcmp(buf, pattern, n * NAND_PAGE_SIZE));
    check("a later sync commits it",
          ftl_sync() == 0 && ftl_init() == 0
          && ftl_read(base, n, buf) == 0
          && !memcmp(buf, pattern, n * NAND_PAGE_SIZE));

    /* Regression: a superblock released by one merge must not be handed
     * straight back to the next merge in the same uncommitted window. If it
     * is, the committed map still names it for the first logical block while
     * it now holds the second one's data, and after an unclean shutdown the
     * first block reads the wrong data while the second is unrecoverable.
     * Least-worn allocation made this reachable; FIFO only hid it.
     * Every block written without a sync must survive the remount. */
    {
        static uint8_t blk[12][NAND_PAGE_SIZE];
        uint32_t first = 200, nblk = 12, k, bad = 0;

        for (k = 0; k < nblk; k++)
            memset(blk[k], (uint8_t)(0x40 + k), NAND_PAGE_SIZE);
        for (k = 0; k < nblk; k++)
            if (ftl_write((first + k) * SBPAGES + 9, 1, blk[k]))
                bad++;
        check("12 single-sector writes to 12 blocks, no sync", bad == 0);
        {
            uint32_t fc, fr, nl;

            ftl_pool_state(&fc, &fr, &nl);
            printf("     %u of them still in uncommitted logs\n", nl);
            check("some of them are still uncommitted", nl > 0);
        }
        check("remounts without a sync", ftl_init() == 0);
        bad = 0;
        for (k = 0; k < nblk; k++)
            if (ftl_read((first + k) * SBPAGES + 9, 1, buf)
                || memcmp(buf, blk[k], NAND_PAGE_SIZE))
                bad++;
        check("every uncommitted block is recovered by the log scan",
              bad == 0);
        check("and a sync commits them all",
              ftl_sync() == 0 && ftl_init() == 0);
        bad = 0;
        for (k = 0; k < nblk; k++)
            if (ftl_read((first + k) * SBPAGES + 9, 1, buf)
                || memcmp(buf, blk[k], NAND_PAGE_SIZE))
                bad++;
        check("all 12 survive the commit", bad == 0);
    }

    /* More merges than the pool has blocks: ftl_write_merge() must commit
     * on its own rather than reuse a block released moments ago. */
    {
        static uint8_t big2[NAND_PAGE_SIZE];
        uint32_t first = 400, nblk = 26, k, bad = 0;

        memset(big2, 0x5c, sizeof(big2));
        for (k = 0; k < nblk; k++)
            if (ftl_write((first + k) * SBPAGES + 11, 1, big2))
                bad++;
        check("26 merges - more than the 20-block pool - succeed", bad == 0);
        check("remount after them", ftl_init() == 0);
        bad = 0;
        for (k = 0; k < nblk; k++)
            if (ftl_read((first + k) * SBPAGES + 11, 1, buf)
                || memcmp(buf, big2, NAND_PAGE_SIZE))
                bad++;
        check("all 26 read back after the automatic commits", bad == 0);
        check("final sync", ftl_sync() == 0 && ftl_init() == 0);
    }

    /* A sync with nothing written since the last commit must not commit
     * again: every shutdown calls it. ftl-nano2g.c returns early on its
     * clean flag in the same way. */
    {
        unsigned long w;

        check("sync before the idle-sync checks", ftl_sync() == 0);
        w = mock_writes;
        check("a sync with nothing written returns 0", ftl_sync() == 0);
        check("... and programs no pages", mock_writes == w);
        check("remount after a clean commit", ftl_init() == 0);
        w = mock_writes;
        check("a sync straight after remounting programs no pages",
              ftl_sync() == 0 && mock_writes == w);
    }

    /* One ftl_write_merge() call long enough to commit on its own partway
     * through, and then keep writing. The commit leaves a clean context, so
     * the dirty mark has to be written again before the later pages, or
     * Apple's firmware would trust that context after an unclean stop and
     * never look for them. */
    {
        static uint8_t pg[NAND_PAGE_SIZE];
        struct every_ctx e = { 600, 20, 7, pg };
        uint32_t usn_before, usn_after, k, bad = 0;
        int tail;

        memset(pg, 0x3b, sizeof(pg));
        check("clean before the long write",
              ftl_sync() == 0 && apple_ctrl_tail(&usn_before) == 0x43);
        check("a long scattered write returns 0",
              ftl_hook_write_merge(e.first * SBPAGES,
                              (e.first + e.nblk) * SBPAGES - 1,
                              every_src, &e) == 0);
        tail = apple_ctrl_tail(&usn_after);
        check("it committed on its own partway through",
              usn_after != usn_before);
        check("pages written after that commit are flagged for Apple (0x4f)",
              tail == 0x4f);
        check("remount without syncing", ftl_init() == 0);
        for (k = 0; k < e.nblk; k++)
            if (ftl_read((e.first + k) * SBPAGES + e.idx, 1, buf)
                || memcmp(buf, pg, NAND_PAGE_SIZE))
                bad++;
        check("every block of the long write reads back", bad == 0);
        check("sync after the long write", ftl_sync() == 0 && ftl_init() == 0);
    }

    check_apple_invariants(usersb);

    printf(fails ? "SOME CHECKS FAILED\n" : "ALL PASS\n");
    return fails ? 1 : 0;
}
