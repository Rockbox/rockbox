/* The FTL on a formatted blank medium of a chosen geometry, for chips no
 * dump exists of - above all 4KiB pages, where one NAND page holds two
 * sectors. Writes of whole superblocks, odd sectors and pages split across
 * two calls must read back, before and after a sync and remount, and after
 * a remount without one; the medium must never be programmed against NAND's
 * rules.
 *
 *   test_format blank:PRESET   (see ftl_hook_open)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nand-target.h"
#include "ftl-target.h"
#include "mock_nand.h"

#define LBSECTORS   1024        /* sectors in a logical block, every preset */
#define BASE        900         /* first logical block used */
#define NLB         8
#define NSEC        (NLB * LBSECTORS)

static uint16_t tag[NSEC];      /* what each sector holds; 0 = never written */
static uint8_t buf[LBSECTORS * NAND_PAGE_SIZE];
static int fails;

static void check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        fails++;
}

static void fill(uint8_t *p, uint32_t sector, uint16_t t)
{
    uint32_t j;

    for (j = 0; j < NAND_PAGE_SIZE; j += 4)
    {
        uint32_t w = sector * 2654435761u ^ t * 40503u ^ j;
        memcpy(p + j, &w, 4);
    }
}

static int put(uint32_t off, uint32_t n, uint16_t t)
{
    uint32_t i;

    for (i = 0; i < n; i++)
        fill(buf + i * NAND_PAGE_SIZE, BASE * LBSECTORS + off + i, t);
    if (ftl_write(BASE * LBSECTORS + off, n, buf))
        return -1;
    for (i = 0; i < n; i++)
        tag[off + i] = t;
    return 0;
}

/* Every sector of the group, one read call per logical block */
static int verify(void)
{
    static uint8_t want[NAND_PAGE_SIZE];
    uint32_t lb, i, bad = 0;

    for (lb = 0; lb < NLB; lb++)
    {
        if (ftl_read((BASE + lb) * LBSECTORS, LBSECTORS, buf))
            return -1;
        for (i = 0; i < LBSECTORS; i++)
        {
            uint32_t s = lb * LBSECTORS + i;

            if (tag[s])
                fill(want, BASE * LBSECTORS + s, tag[s]);
            else
                memset(want, 0xff, NAND_PAGE_SIZE);
            if (memcmp(buf + i * NAND_PAGE_SIZE, want, NAND_PAGE_SIZE))
                bad++;
        }
    }
    if (bad)
        printf("     %u sectors wrong\n", bad);
    return bad ? -1 : 0;
}

/* Single sectors one call at a time, and odd runs, read back singly */
static int verify_single(uint32_t off, uint32_t n)
{
    static uint8_t one[3 * NAND_PAGE_SIZE], want[NAND_PAGE_SIZE];
    uint32_t i, k, len;

    for (i = 0; i < n; i += len)
    {
        len = 1 + (i % 3);
        if (i + len > n)
            len = n - i;
        if (ftl_read(BASE * LBSECTORS + off + i, len, one))
            return -1;
        for (k = 0; k < len; k++)
        {
            uint32_t s = off + i + k;
            if (tag[s])
                fill(want, BASE * LBSECTORS + s, tag[s]);
            else
                memset(want, 0xff, NAND_PAGE_SIZE);
            if (memcmp(one + k * NAND_PAGE_SIZE, want, NAND_PAGE_SIZE))
                return -1;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    const char *spec = argc > 1 ? argv[1] : "blank:4k2";
    char msg[256];
    uint32_t i, r = 12345;
    uint16_t t = 1;
    int ok;

    if (ftl_hook_open(spec))
    {
        printf("FAIL  format %s\n", spec);
        return 1;
    }
    printf("     %s: %u banks, %u-byte pages\n", spec, mock_geo.banks,
           mock_geo.pagesize);
    check("format programs every page in order",
          mock_nonsequential == 0 && mock_reprogram == 0);
    mock_reset_stats();
    check("formatted volume mounts", ftl_init() == 0);
    check("... writable", ftl_hook_writeable());
    check("... sector count", ftl_num_sectors()
          == mock_geo.userblocks / mock_geo.planes * LBSECTORS);
    check("unwritten sectors read erased", verify() == 0);

    check("whole logical block", put(0, LBSECTORS, t++) == 0);
    check("one sector at the start of a page", put(LBSECTORS + 10, 1, t++)
          == 0);
    check("one sector at the end of a page", put(LBSECTORS + 21, 1, t++)
          == 0);
    check("an odd run crossing pages", put(LBSECTORS + 101, 13, t++) == 0);
    check("an odd run crossing logical blocks",
          put(2 * LBSECTORS - 7, 15, t++) == 0);
    check("two halves of one page in two calls",
          put(3 * LBSECTORS + 50, 1, t++) == 0
          && put(3 * LBSECTORS + 51, 1, t++) == 0);
    check("reads back", verify() == 0);
    check("reads back a sector at a time", verify_single(LBSECTORS, 200) == 0
          && verify_single(2 * LBSECTORS - 16, 40) == 0);

    /* Random small writes, enough to fill and merge logs */
    ok = 1;
    for (i = 0; i < 3000 && ok; i++)
    {
        uint32_t n, off;

        r = r * 1103515245u + 12345u;
        n = 1 + (r >> 16) % 12;
        r = r * 1103515245u + 12345u;
        off = (r >> 8) % (NSEC - n);
        ok = put(off, n, t++) == 0;
    }
    check("3000 random writes of 1-12 sectors", ok);
    check("... read back", verify() == 0);
    check("sync", ftl_sync() == 0);
    check("remount", ftl_init() == 0);
    check("... read back", verify() == 0);
    check("... consistent", ftl_hook_check(msg, sizeof(msg)) == 0);

    /* Writes left uncommitted are recovered by the restore */
    ok = 1;
    for (i = 0; i < 500 && ok; i++)
    {
        uint32_t n, off;

        r = r * 1103515245u + 12345u;
        n = 1 + (r >> 16) % 5;
        r = r * 1103515245u + 12345u;
        off = (r >> 8) % (NSEC - n);
        ok = put(off, n, t++) == 0;
    }
    check("500 more writes, no sync", ok);
    check("remount without sync", ftl_init() == 0);
    check("... read back", verify() == 0);
    check("... consistent", ftl_hook_check(msg, sizeof(msg)) == 0
          || (printf("     %s\n", msg), 0));

    check("no page programmed twice", mock_reprogram == 0);
    check("no page programmed out of order", mock_nonsequential == 0);
    check("no two-plane pair the chip could not take",
          mock_bad_twoplane == 0);

    /* A bad spare before a replaced block must still mount; superblock
     * 1500 holds none of the data. */
    check("a bad spare then a remap commits",
          ftl_hook_bad_spare_then_remap(0, 1500) == 0);
    check("... and still mounts", ftl_init() == 0);
    check("... and reads back", verify() == 0);

    /* Apple's pending-bad list: the next erase replaces the block */
    {
        bool moved = false;
        check("a pending block is replaced at its erase",
              ftl_hook_pending_erase(1, 0, 1600, &moved) == 0 && moved);
        /* now its spare is pending: replacing it must clear the entry */
        moved = false;
        check("a pending spare is replaced and leaves the list",
              ftl_hook_pending_erase(1, 0, 1600, &moved) == 0 && moved);
        check("... and still mounts", ftl_init() == 0);
        check("... and reads back", verify() == 0);
    }
    check("VFL commits program every page in order",
          mock_nonsequential == 0 && mock_reprogram == 0);

    /* The policy that keeps unvalidated chips safe, in this layout: the
     * mount reads but never programs or erases */
    {
        const struct nand_geometry good = mock_geo;
        unsigned long writes, erases;
        static uint8_t one[NAND_PAGE_SIZE];

        mock_geo.validated = false;
        check("an unvalidated row mounts", ftl_init() == 0);
        check("... read-only", !ftl_hook_writeable());
        check("... and reads", ftl_read(0, 1, one) == 0);
        writes = mock_writes;
        erases = mock_erases;
        memset(one, 0x5a, sizeof(one));
        check("... refuses a write", ftl_write(0, 1, one) != 0);
        check("... sync does nothing", ftl_sync() == 0);
        check("... no program or erase reached the NAND",
              mock_writes == writes && mock_erases == erases);
        mock_geo = good;
        check("the validated row mounts writable",
              ftl_init() == 0 && ftl_hook_writeable());
    }

    puts(fails ? "SOME FAILED" : "ALL PASS");
    return fails != 0;
}
