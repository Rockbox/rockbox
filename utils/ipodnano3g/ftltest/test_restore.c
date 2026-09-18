/* Closed data blocks the committed map does not know about.
 *
 * Apple's disk mode writes whole logical blocks - closed superblocks, last
 * page type 0x41 - without committing its block map, so after a large copy
 * the committed map is older than dozens of them. Apple's own _FTLRestore
 * puts every closed block into the map, the highest usn winning, and treats
 * only unclosed ones as logs. Found on the device: after a 77 MB copy in
 * Apple's disk mode, Rockbox's mount failed with "more than 17 log blocks".
 *
 * Simulated here by writing whole blocks through our own FTL, with its
 * commits, and then rolling the three control superblocks back to their
 * earlier contents: the committed map and pool are old again, and the
 * blocks written since are closed but unmapped.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nand-target.h"
#include "ftl-target.h"
#include "mock_nand.h"

#define SBPAGES 1024
#define PAGES   (4096UL * 128)     /* per bank, as mock_nand.c lays them out */
#define METASZ  16
#define BASE    1500
#define NBLK    25

static int fails;

static void check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        fails++;
}

static void fill(uint8_t *buf, uint32_t lblock)
{
    uint32_t i;

    for (i = 0; i < SBPAGES * NAND_PAGE_SIZE; i++)
        buf[i] = (uint8_t)(i * 7 + lblock * 13 + 1);
}

/* Copy every physical page of the control superblocks out or back in */
static void ctrl_pages(uint16_t *ctrl, uint8_t *data, uint8_t *meta, int save)
{
    uint32_t i, v, bank, page;
    size_t g, k = 0;

    for (i = 0; i < 3; i++)
        for (v = 0; v < SBPAGES; v++, k++)
        {
            if (ftl_sb_locate(ctrl[i], v, &bank, &page))
            {
                puts("locate failed");
                exit(1);
            }
            g = bank * PAGES + page;
            if (save)
            {
                memcpy(data + k * NAND_PAGE_SIZE,
                       mock_data + g * NAND_PAGE_SIZE, NAND_PAGE_SIZE);
                memcpy(meta + k * METASZ, mock_meta + g * METASZ, METASZ);
            }
            else
            {
                memcpy(mock_data + g * NAND_PAGE_SIZE,
                       data + k * NAND_PAGE_SIZE, NAND_PAGE_SIZE);
                memcpy(mock_meta + g * METASZ, meta + k * METASZ, METASZ);
            }
        }
}

int main(int argc, char **argv)
{
    static uint8_t blk[SBPAGES * NAND_PAGE_SIZE], buf[SBPAGES * NAND_PAGE_SIZE];
    static uint8_t cdata[3 * SBPAGES * NAND_PAGE_SIZE], cmeta[3 * SBPAGES * METASZ];
    uint16_t ctrl[3];
    uint32_t k, bad;
    int ok = 1, rc;

    mock_open(argc > 1 ? argv[1] : "../n3g-dump");
    if (ftl_init()) { puts("mount failed"); return 1; }
    check("settles", ftl_hook_settle() == 0);
    /* A write before anything else marks the context dirty (0x4f), as
     * Apple's disk mode does; that mark is part of what gets rolled back */
    {
        static uint8_t s0[NAND_PAGE_SIZE];
        check("a write marks the context dirty",
              ftl_read(0, 1, s0) == 0 && ftl_write(0, 1, s0) == 0);
    }

    ftl_hook_ctrl(ctrl);
    ctrl_pages(ctrl, cdata, cmeta, 1);
    printf("     control superblocks %u %u %u saved\n", ctrl[0], ctrl[1], ctrl[2]);

    for (k = 0; k < NBLK && ok; k++)
    {
        fill(blk, BASE + k);
        ok = ftl_write((BASE + k) * SBPAGES, SBPAGES, blk) == 0;
    }
    check("25 whole-block writes", ok);
    check("committed", ftl_sync() == 0);

    ctrl_pages(ctrl, cdata, cmeta, 0);
    puts("     control superblocks rolled back: the committed map predates them");

    rc = ftl_init();
    printf("     ftl_init() = %d\n", rc);
    check("mounts with 25 closed blocks the committed map lacks", rc == 0);
    if (rc)
    {
        printf("SOME CHECKS FAILED\n");
        return 1;
    }

    bad = 0;
    for (k = 0; k < NBLK; k++)
    {
        fill(blk, BASE + k);
        if (ftl_read((BASE + k) * SBPAGES, SBPAGES, buf) || memcmp(buf, blk, sizeof(blk)))
            bad++;
    }
    printf("     %u of %u blocks read wrong\n", bad, NBLK);
    check("every closed block is read", bad == 0);
    check("writable", ftl_hook_writeable());

    /* A write, a commit and a clean remount must keep all of it */
    memset(buf, 0x5a, NAND_PAGE_SIZE);
    check("a small write after the mount", ftl_write(BASE * SBPAGES + 3, 1, buf) == 0);
    check("sync", ftl_sync() == 0);
    check("clean remount", ftl_init() == 0);
    bad = 0;
    for (k = 0; k < NBLK; k++)
    {
        fill(blk, BASE + k);
        if (k == 0)
            memset(blk + 3 * NAND_PAGE_SIZE, 0x5a, NAND_PAGE_SIZE);
        if (ftl_read((BASE + k) * SBPAGES, SBPAGES, buf) || memcmp(buf, blk, sizeof(blk)))
            bad++;
    }
    printf("     %u of %u blocks read wrong after the commit\n", bad, NBLK);
    check("all of it survives a commit and remount", bad == 0);
    check("NAND rules kept", mock_reprogram == 0 && mock_nonsequential == 0);

    printf(fails ? "SOME CHECKS FAILED\n" : "ALL PASS\n");
    return fails ? 1 : 0;
}
