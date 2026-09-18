/* Unclean-stop safety of pool allocation: a block released since the last
 * commit is still the committed map's copy of some logical block, so
 * ftl_pool_alloc() must never have to hand one out. ftl_write_merge()
 * commits early to guarantee that; its bound has to count open logs too,
 * since they occupy pool blocks. The write sequence below drives the pool
 * to "no block free at the last commit" without tripping a bound that
 * ignores ftl_nlogs.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nand-target.h"
#include "ftl-target.h"
#include "mock_nand.h"

#define SBPAGES 1024
#define BASE    1000        /* first logical block used */

static int fails;

static void check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        fails++;
}

static void fill(uint8_t *buf, uint32_t lpn, uint32_t n, uint8_t salt)
{
    uint32_t i, j;

    for (i = 0; i < n; i++)
        for (j = 0; j < NAND_PAGE_SIZE; j++)
            buf[i * NAND_PAGE_SIZE + j] = (uint8_t)(lpn + i + j * 3 + salt);
}

/* Blocks the next allocation may take without breaking the invariant */
static uint32_t safe_free(void)
{
    uint32_t freecount, fresh, nlogs;

    ftl_pool_state(&freecount, &fresh, &nlogs);
    printf("     free %u fresh %u logs %u\n", freecount, fresh, nlogs);
    return freecount > fresh ? freecount - fresh : 0;
}

int main(int argc, char **argv)
{
    static uint8_t blk[SBPAGES * NAND_PAGE_SIZE], buf[SBPAGES * NAND_PAGE_SIZE];
    uint32_t i, lpn, minsafe = 0xffffffff, bad = 0;
    int ok = 1;

    mock_open(argc > 1 ? argv[1] : "../n3g-dump");
    if (ftl_init()) { puts("mount failed"); return 1; }
    check("mounts writeable", ftl_hook_writeable());
    check("settles", ftl_hook_settle() == 0);

    /* Eight blocks that each get a log and then a whole-block write, which
     * folds the log: two releases per block */
    for (i = 0; i < 8 && ok; i++)
    {
        lpn = (BASE + i) * SBPAGES;
        fill(blk, lpn + 5, 16, 1);
        ok = ftl_write(lpn + 5, 16, blk) == 0;
        fill(blk, lpn, SBPAGES, 2);
        ok = ok && ftl_write(lpn, SBPAGES, blk) == 0;
    }
    check("eight log-then-whole-block rewrites", ok);

    /* One log filled in order over two calls, adopted outright: one release */
    lpn = (BASE + 8) * SBPAGES;
    fill(blk, lpn, SBPAGES, 3);
    check("a sequential log filled in two halves",
          ftl_write(lpn, SBPAGES / 2, blk) == 0
          && ftl_write(lpn + SBPAGES / 2, SBPAGES / 2,
                       blk + SBPAGES / 2 * NAND_PAGE_SIZE) == 0);

    /* Open logs on four more blocks. Each takes a pool block. */
    for (i = 9; i < 13 && ok; i++)
    {
        uint32_t s = safe_free();

        if (s < minsafe)
            minsafe = s;
        lpn = (BASE + i) * SBPAGES;
        fill(blk, lpn + 7, 16, 4);
        ok = ftl_write(lpn + 7, 16, blk) == 0;
    }
    check("four small writes opening logs", ok);
    printf("     fewest safe blocks before an allocation: %u\n", minsafe);

    /* And an unclean stop now must lose nothing */
    check("remount without syncing", ftl_init() == 0);
    for (i = 0; i < 8; i++)
    {
        lpn = (BASE + i) * SBPAGES;
        fill(blk, lpn, SBPAGES, 2);
        if (ftl_read(lpn, SBPAGES, buf) || memcmp(buf, blk, sizeof(blk)))
            bad++;
    }
    lpn = (BASE + 8) * SBPAGES;
    fill(blk, lpn, SBPAGES, 3);
    if (ftl_read(lpn, SBPAGES, buf) || memcmp(buf, blk, sizeof(blk)))
        bad++;
    for (i = 9; i < 13; i++)
    {
        lpn = (BASE + i) * SBPAGES;
        fill(blk, lpn + 7, 16, 4);
        if (ftl_read(lpn + 7, 16, buf) || memcmp(buf, blk, 16 * NAND_PAGE_SIZE))
            bad++;
    }
    printf("     %u of 13 blocks wrong after the unclean remount\n", bad);
    check("everything written reads back after an unclean stop", bad == 0);

    check("final sync", ftl_sync() == 0);
    printf(fails ? "SOME CHECKS FAILED\n" : "ALL PASS\n");
    return fails ? 1 : 0;
}
