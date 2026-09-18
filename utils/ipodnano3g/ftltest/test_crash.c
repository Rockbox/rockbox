/* Power-loss stress: random writes and syncs over a group of logical blocks,
 * with the mock cutting power at a random program or erase - in the middle
 * of a log append, a fold, a sequential adoption, a wear-levelling move or a
 * commit. After each cut the FTL is mounted afresh and every sector in the
 * group must hold what the last acknowledged write put there; a sector in
 * the write that was cut may hold either its old or its new contents.
 *
 *   test_crash [DUMPDIR] [CUTS] [SEED] [MAXOPS] [TORN]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nand-target.h"
#include "ftl-target.h"
#include "mock_nand.h"

#define SBPAGES 1024
#define BASE    1000                    /* first logical block used */
#define NLB     40                      /* logical blocks used */
#define NLPN    (NLB * SBPAGES)
#define CHUNK   12                      /* Rockbox's USB write size */

static uint32_t model[NLPN];            /* tag of each sector, 0 = unwritten */
static uint32_t pend_lo, pend_n, pend_tag, pend_base;
static uint8_t page[CHUNK * NAND_PAGE_SIZE];
static uint32_t rnd_state;

static uint32_t rnd(void)
{
    rnd_state = rnd_state * 1103515245u + 12345u;
    return rnd_state >> 8;
}

static uint32_t tag_of(const uint8_t *p)
{
    uint32_t t;
    memcpy(&t, p, 4);
    return t;
}

/* Write n sectors from lpn, tag = (write id << 12) + offset, in USB chunks */
static int do_write(uint32_t lo, uint32_t n, uint32_t id)
{
    uint32_t done, k, c;

    pend_lo = lo;
    pend_n = n;
    pend_tag = id;
    pend_base = lo;
    for (done = 0; done < n; done += c)
    {
        c = n - done < CHUNK ? n - done : CHUNK;
        for (k = 0; k < c; k++)
        {
            uint32_t t = (id << 12) + done + k, w;
            for (w = 0; w < NAND_PAGE_SIZE; w += 4)
                memcpy(page + k * NAND_PAGE_SIZE + w, &t, 4);
        }
        {
            int rc = ftl_write(BASE * SBPAGES + lo + done, c, page);
            if (rc)
            {
                printf("     ftl_write(+%u, %u) = %d\n", lo + done, c, rc);
                return -1;
            }
        }
        /* Acknowledged: this chunk is now owed to the host */
        for (k = 0; k < c; k++)
            model[lo + done + k] = (id << 12) + done + k;
        pend_lo = lo + done + c;
        pend_n = n - done - c;
    }
    pend_n = 0;
    return 0;
}

static int verify(uint32_t *bad_out, uint32_t *ambig_out)
{
    static uint8_t buf[NAND_PAGE_SIZE];
    uint32_t i, got, bad = 0, ambig = 0;

    for (i = 0; i < NLPN; i++)
    {
        if (ftl_read(BASE * SBPAGES + i, 1, buf))
        {
            bad++;
            continue;
        }
        got = tag_of(buf);
        if (model[i] == 0 && got == 0xffffffff)
            got = 0;
        if (got == model[i])
            continue;
        if (pend_n && i >= pend_lo && i < pend_lo + pend_n
            && got == (pend_tag << 12) + (i - pend_base))
        {
            ambig++;
            model[i] = got;
            continue;
        }
        if (bad < 5)
            printf("     lpn +%u: got %08x want %08x\n", i, got, model[i]);
        bad++;
    }
    *bad_out = bad;
    *ambig_out = ambig;
    return bad ? -1 : 0;
}

int main(int argc, char **argv)
{
    static unsigned long cuts, cut;
    static uint32_t id = 1, bad, ambig, totalbad, syncs, writes;
    static int fails;

    static unsigned long maxops;

    cuts = argc > 2 ? strtoul(argv[2], NULL, 0) : 300;
    maxops = argc > 4 ? strtoul(argv[4], NULL, 0) : 6000;
    mock_crash_torn = argc > 5 ? atoi(argv[5]) : 0;

    rnd_state = argc > 3 ? strtoul(argv[3], NULL, 0) : 12345;
    if (ftl_hook_open(argc > 1 ? argv[1] : "../n3g-dump"))
    {
        puts("cannot open the medium");
        return 2;
    }
    if (ftl_init() || ftl_sync() || ftl_init())
    {
        puts("mount failed");
        return 1;
    }
    /* Start from known contents: every sector of the group written once */
    {
        uint32_t lb;
        for (lb = 0; lb < NLB; lb++)
            if (do_write(lb * SBPAGES, SBPAGES, id++))
            {
                puts("initial fill failed");
                return 1;
            }
        if (ftl_sync())
        {
            puts("initial sync failed");
            return 1;
        }
    }

    for (cut = 0; cut < cuts; cut++)
    {
        mock_crash_after = 1 + rnd() % maxops;
        if (!setjmp(mock_crash_jmp))
        {
            for (;;)
            {
                uint32_t r = rnd() % 100, lb = rnd() % NLB;

                if (r < 75)                   /* scattered small write */
                {
                    uint32_t n = 1 + rnd() % 40;
                    uint32_t lo = lb * SBPAGES + rnd() % (SBPAGES - n);
                    if (do_write(lo, n, id++)) break;
                    writes++;
                }
                else if (r < 90)              /* a stretch written in order */
                {
                    uint32_t n = 64 + rnd() % 900;
                    uint32_t lo = lb * SBPAGES + rnd() % (SBPAGES - n);
                    if (do_write(lo, n, id++)) break;
                    writes++;
                }
                else if (r < 95)              /* a whole block */
                {
                    if (do_write(lb * SBPAGES, SBPAGES, id++)) break;
                    writes++;
                }
                else
                {
                    { int rc = ftl_sync(); if (rc) { printf("     ftl_sync() = %d\n", rc); break; } }
                    syncs++;
                }
            }
            printf("     cut %lu: a write or sync failed without a cut\n", cut);
            fails++;
            break;
        }
        /* Power was cut. Mount afresh and check. */
        mock_crash_after = 0;
        if (ftl_init())
        {
            printf("     cut %lu: mount failed\n", cut);
            fails++;
            break;
        }
        {
            char msg[256];
            if (ftl_hook_check(msg, sizeof(msg)))
            {
                int k;
                printf("     cut %lu: FTL state inconsistent after remount: %s\n", cut, msg);
                printf("     the cut was at:");
                for (k = 0; k < mock_crash_btn; k++)
                    printf(" %p", mock_crash_bt[k]);
                printf("\n");
                fails++;
                break;
            }
        }
        verify(&bad, &ambig);
        if (bad)
        {
            int k;
            printf("     cut %lu: %u sectors wrong; pending +%u n %u tag %u\n", cut, bad, pend_base, pend_n, pend_tag);
            printf("     the cut was at:");
            for (k = 0; k < mock_crash_btn; k++)
                printf(" %p", mock_crash_bt[k]);
            printf("\n");
            totalbad += bad;
            fails++;
            break;
        }
        pend_n = 0;
    }
    printf("     %lu cuts, %u writes, %u syncs, %u sectors wrong\n",
           cut, writes, syncs, totalbad);
    printf("%s  every acknowledged write survives every cut\n",
           fails ? "FAIL" : "PASS");
    printf("NAND rules: reprogram %lu, nonsequential %lu\n",
           mock_reprogram, mock_nonsequential);
    printf(fails ? "SOME CHECKS FAILED\n" : "ALL PASS\n");
    return fails ? 1 : 0;
}
