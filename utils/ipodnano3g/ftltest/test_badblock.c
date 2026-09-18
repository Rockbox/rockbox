/* Bad-block handling: a physical block that refuses programs or erases must
 * be replaced from the VFL's reserved spares, the write must still succeed,
 * and the replacement must survive a remount - which only happens if the VFL
 * context was committed. Ported alongside ftl-nano2g.c's VFL write side.
 */
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
#define SBPAGES (PPB * PLANES * BANKS)
#define POOL    20

static int fails;

static void check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        fails++;
}

/* The physical (bank, block) behind page v of superblock sb */
static int phys_of(uint32_t sb, uint32_t v, uint32_t *bank, uint32_t *block)
{
    uint32_t page;

    if (ftl_sb_locate(sb, v, bank, &page))
        return -1;
    *block = page / PPB;
    return 0;
}

int main(int argc, char **argv)
{
    static uint8_t blk[SBPAGES * NAND_PAGE_SIZE];   /* one superblock */
    static uint8_t buf[SBPAGES * NAND_PAGE_SIZE];
    uint16_t psb[POOL], pec[POOL];
    uint32_t n, i, victim = 0, vlow = 0xffff, bank = 0, block = 0, lblock = 800;
    uint32_t vblock;
    int entry_before, entry_after;

    mock_open(argc > 1 ? argv[1] : "../n3g-dump");
    if (ftl_init()) { puts("mount failed"); return 1; }
    check("mounts writeable", ftl_hook_writeable());
    check("settles", ftl_hook_settle() == 0);

    /* The next allocation takes the least-worn pool block, so that is the
     * one a write will land in - make its first physical block refuse
     * programs, as a worn-out block does. */
    n = ftl_pool_snapshot(psb, pec, POOL);
    check("pool snapshot", n == POOL);
    for (i = 0; i < n; i++)
        if (pec[i] < vlow) { vlow = pec[i]; victim = psb[i]; }
    check("found the block the next write will use", victim != 0);
    check("located it physically", phys_of(victim, 0, &bank, &block) == 0);
    vblock = victim * PLANES;           /* v=0 is plane 0 of the superblock */
    entry_before = ftl_vfl_pool_entry(bank, block);
    printf("     victim sb %u -> bank %u block %u (pool entry %d, ec %u)\n",
           victim, bank, block, entry_before, vlow);

    mock_clear_bad();
    mock_set_bad(bank, block, MOCK_FAIL_WRITE);

    /* A whole-block write, so it goes through ftl_merge()'s retry path.
     * A failed program into a live log panics by design instead. */
    for (i = 0; i < sizeof(blk); i++)
        blk[i] = (uint8_t)(i * 7 + 3);
    check("a write into a block that refuses programs still succeeds",
          ftl_write(lblock * SBPAGES, SBPAGES, blk) == 0);
    check("the mock really did refuse a program", mock_write_fails > 0);
    check("the data reads back",
          ftl_read(lblock * SBPAGES, SBPAGES, buf) == 0
          && !memcmp(buf, blk, sizeof(blk)));

    /* The bad virtual block must now be remapped onto a reserved spare */
    {
        int found = -1;
        uint32_t b;

        for (b = 0; b < BLOCKS; b++)
            if (ftl_vfl_pool_entry(bank, b) == (int)vblock)
            {
                found = b;
                break;
            }
        printf("     vblock %u is now served by physical block %d\n",
               vblock, found);
        check("the bad block was remapped onto a spare", found > 0);
    }

    /* ...and the remap has to survive a remount, which means the VFL
     * context was committed to flash */
    check("sync and remount", ftl_sync() == 0 && ftl_init() == 0);
    {
        int found = -1;
        uint32_t b;

        for (b = 0; b < BLOCKS; b++)
            if (ftl_vfl_pool_entry(bank, b) == (int)vblock)
            {
                found = b;
                break;
            }
        check("the remap survived the remount (VFL context committed)",
              found > 0);
    }
    entry_after = ftl_vfl_pool_entry(bank, block);
    check("the old slot is unchanged in the pool table",
          entry_after == entry_before);
    check("the data still reads back after the remount",
          ftl_read(lblock * SBPAGES, SBPAGES, buf) == 0
          && !memcmp(buf, blk, sizeof(blk)));

    /* A block that refuses erases must be replaced too. Erases happen when
     * the *old* superblock is released after a merge, so the block to kill
     * is the one that currently holds the logical block being written. */
    mock_clear_bad();
    {
        uint32_t page2;

        check("locate the block about to be released",
              ftl_locate((lblock + 1) * SBPAGES, &bank, &page2) == 0);
        block = page2 / PPB;
        printf("     lblock %u currently lives in bank %u block %u\n",
               lblock + 1, bank, block);
        mock_set_bad(bank, block, MOCK_FAIL_ERASE);
        for (i = 0; i < sizeof(blk); i++)
            blk[i] = (uint8_t)(i * 13 + 1);
        /* The erase comes when the old superblock is released, which for a
         * partial write happens at the sync that folds the log. */
        check("a write that must erase a dead block still succeeds",
              ftl_write((lblock + 1) * SBPAGES, 16, blk) == 0
              && ftl_sync() == 0);
        check("the mock really did refuse an erase", mock_erase_fails > 0);
        check("that data reads back too",
              ftl_read((lblock + 1) * SBPAGES, 16, buf) == 0
              && !memcmp(buf, blk, 16 * NAND_PAGE_SIZE));
    }

    mock_clear_bad();
    check("final sync and remount", ftl_sync() == 0 && ftl_init() == 0);
    check("everything still readable",
          ftl_read(lblock * SBPAGES, 16, buf) == 0);

    printf(fails ? "SOME CHECKS FAILED\n" : "ALL PASS\n");
    return fails ? 1 : 0;
}
