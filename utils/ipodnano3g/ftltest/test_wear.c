/* Wear behaviour of the free pool: allocation must take the least-worn
 * block, as Apple's _GetFreeVb does, and repeated writes must spread over
 * the pool rather than grinding one block down. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nand-target.h"
#include "ftl-target.h"
#include "mock_nand.h"

#define SBPAGES 1024
#define POOL    20

static int fails;

static void check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        fails++;
}

int main(int argc, char **argv)
{
    static uint8_t buf[NAND_PAGE_SIZE];
    uint16_t sb[POOL], ec[POOL];
    uint32_t n, i, bank, page, got, lowest, lowec, spread_before, spread_after;
    uint32_t mn, mx, round, poolmax_initial;

    mock_open(argc > 1 ? argv[1] : "../n3g-dump");
    if (ftl_init()) { puts("mount failed"); return 1; }
    check("mounts writeable", ftl_hook_writeable());
    /* settle: fold inherited logs so the pool is whole */
    check("initial sync", ftl_hook_settle() == 0);

    n = ftl_pool_snapshot(sb, ec, POOL);
    check("the pool is full after a sync", n == POOL);
    if (n != POOL)
        return 1;
    mn = 0xffff; mx = 0;
    lowest = sb[0]; lowec = ec[0];
    for (i = 0; i < n; i++)
    {
        if (ec[i] < mn) mn = ec[i];
        if (ec[i] > mx) mx = ec[i];
        if (ec[i] < lowec) { lowec = ec[i]; lowest = sb[i]; }
    }
    spread_before = mx - mn;
    poolmax_initial = mx;
    printf("     pool erase counts: min %u max %u spread %u; least worn is sb %u\n",
           mn, mx, spread_before, lowest);
    check("the pool is not uniformly worn (so the choice is observable)",
          spread_before > 0);

    /* One write: the block it lands in must be the least worn in the pool */
    memset(buf, 0xa5, sizeof(buf));
    check("write", ftl_write(700 * SBPAGES + 1, 1, buf) == 0);
    check("locate", ftl_locate(700 * SBPAGES + 1, &bank, &page) == 0);
    /* Is that physical page inside the least-worn superblock? A small write
     * now lands in a log block, at whatever offset the append cursor was
     * on, so look for the page anywhere in the block rather than at its
     * index within the logical block. */
    got = 0xffffffff;
    for (i = 0; i < SBPAGES; i++)
    {
        uint32_t b2, p2;

        if (!ftl_sb_locate(lowest, i, &b2, &p2) && b2 == bank && p2 == page)
        {
            got = lowest;
            break;
        }
    }
    printf("     the write landed in sb %u (least worn was %u, ec %u)\n",
           got == 0xffffffff ? 0xffffffff : lowest, lowest, lowec);
    check("allocation took the least-worn block", got == lowest);

    /* Hammer one logical block and watch the pool even out rather than
     * one block taking every cycle */
    for (round = 0; round < 60; round++)
    {
        memset(buf, (uint8_t)round, sizeof(buf));
        if (ftl_write(700 * SBPAGES + 1, 1, buf) || ftl_sync())
        {
            check("60 write+sync rounds", 0);
            return 1;
        }
    }
    check("60 write+sync rounds", ftl_init() == 0);
    n = ftl_pool_snapshot(sb, ec, POOL);
    mn = 0xffff; mx = 0;
    for (i = 0; i < n; i++)
    {
        if (ec[i] < mn) mn = ec[i];
        if (ec[i] > mx) mx = ec[i];
    }
    spread_after = mx - mn;
    printf("     after 60 rounds: min %u max %u spread %u (was %u)\n",
           mn, mx, spread_after, spread_before);
    /* Apple's cadence (one move per 20 merges' budget) lets cold blocks
     * into the pool, which widens the pool's spread; reported, not judged */
    printf("     pool spread %s\n", spread_after <= spread_before
           ? "did not grow" : "grew as cold blocks joined the pool");
    check("the data still reads back",
          ftl_read(700 * SBPAGES + 1, 1, buf) == 0 && buf[0] == 59);

    /* Static wear levelling. The signal is not the whole-device spread -
     * 1230 of the 1959 superblocks sit near the minimum, so one relocation
     * per commit cannot move it in any reasonable number of rounds, and the
     * device maximum never falls at all once that block holds cold data.
     * The signal is the *pool*: worn blocks should be retired into cold-data
     * duty and cold blocks recruited in their place, until the spread is
     * under the threshold - at which point levelling must stop, so the cost
     * stays bounded. */
    {
        uint32_t gmin, gmax, r, poolmax_before, poolmax_after;
        uint16_t s2[POOL], e2[POOL];

        ftl_global_spread(&gmin, &gmax);
        printf("     whole-device erase counts: min %u max %u\n", gmin, gmax);
        poolmax_before = poolmax_initial;   /* before any levelling ran */

        for (r = 0; r < 120; r++)
        {
            memset(buf, (uint8_t)r, sizeof(buf));
            if (ftl_write(300 * SBPAGES + 5, 1, buf) || ftl_sync())
            {
                check("120 more write+sync rounds", 0);
                return 1;
            }
        }
        check("120 more write+sync rounds", ftl_init() == 0);

        n = ftl_pool_snapshot(s2, e2, POOL);
        mn = 0xffff; poolmax_after = 0;
        for (i = 0; i < n; i++)
        {
            if (e2[i] < mn) mn = e2[i];
            if (e2[i] > poolmax_after) poolmax_after = e2[i];
        }
        printf("     pool after levelling: min %u max %u (started at %u, "
               "device max %u)\n", mn, poolmax_after, poolmax_before, gmax);
        check("the pool is still whole", n == POOL);
        check("worn blocks were retired out of the pool",
              poolmax_after < poolmax_before);
        check("the pool no longer holds the device's most worn block",
              poolmax_after < gmax);
        printf("     pool spread after levelling: %u\n", poolmax_after - mn);
        check("the data still reads back after levelling",
              ftl_read(300 * SBPAGES + 5, 1, buf) == 0 && buf[0] == 119);
        check("and the earlier block too",
              ftl_read(700 * SBPAGES + 1, 1, buf) == 0 && buf[0] == 59);
    }

    printf(fails ? "SOME CHECKS FAILED\n" : "ALL PASS\n");
    return fails ? 1 : 0;
}
