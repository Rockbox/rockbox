/* What one flush costs, in page programs and block erases, for a few write
 * shapes. Amplification is measured against the bytes the caller asked to
 * write. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nand-target.h"
#include "ftl-target.h"
#include "mock_nand.h"

#define SBPAGES 1024

static uint8_t pat[NAND_PAGE_SIZE];
static uint32_t *lpns;
static uint32_t nlpn;

static const void *src(uint32_t lpn, void *ctx)
{
    uint32_t i;

    (void)ctx;
    for (i = 0; i < nlpn; i++)
        if (lpns[i] == lpn)
            return pat;
    return NULL;
}

#define ROUNDS 10

static void bench(const char *what, uint32_t n, uint32_t stride, uint32_t base)
{
    uint32_t i, r;
    double amp;

    nlpn = n;
    for (i = 0; i < n; i++)
        lpns[i] = base + i * stride;
    mock_reset_stats();
    for (r = 0; r < ROUNDS; r++)
        if (ftl_hook_write_merge(lpns[0], lpns[n - 1], src, NULL) || ftl_sync())
        {
            printf("  %-38s FAILED\n", what);
            return;
        }
    amp = (double)mock_writes / ROUNDS / n;
    printf("  %-38s %7.0f pages, %5.1f block erases, %6.0fx\n", what,
           (double)mock_writes / ROUNDS, (double)mock_erases / ROUNDS, amp);
}

int main(int argc, char **argv)
{
    uint32_t i;

    mock_open(argc > 1 ? argv[1] : "../n3g-dump");
    if (ftl_init()) { puts("mount failed"); return 1; }
    lpns = malloc(4096 * sizeof(uint32_t));
    for (i = 0; i < NAND_PAGE_SIZE; i++)
        pat[i] = (uint8_t)i;
    /* Settle first. A freshly mounted unit has an erase-count spread of
     * about 110, so static wear levelling relocates one cold block per
     * commit until the spread is under the threshold; that is a real cost
     * but a transient one, and it would otherwise be charged to every row
     * below. Run until it stops firing. */
    {
        uint16_t psb[20], pec[20];
        uint32_t r, n, i, mn, mx;

        ftl_sync();
        ftl_init();
        for (r = 0; r < 200; r++)
        {
            n = ftl_pool_snapshot(psb, pec, 20);
            mn = 0xffff; mx = 0;
            for (i = 0; i < n; i++)
            {
                if (pec[i] < mn) mn = pec[i];
                if (pec[i] > mx) mx = pec[i];
            }
            if (n && mx - mn <= 64)
                break;
            lpns[0] = 900 * SBPAGES + 1;
            nlpn = 1;
            if (ftl_hook_write_merge(lpns[0], lpns[0], src, NULL) || ftl_sync())
                break;
        }
        printf("settled after %u levelling rounds "
               "(pool erase counts %u..%u)\n\n", r, mn, mx);
        ftl_init();
    }

    printf("cost of one flush, averaged over %d, including the 25-page\n"
           "commit and any wear-levelling relocation it triggers\n", ROUNDS);
    printf("  %-38s %7s  %5s  %6s\n", "", "written", "erased", "amp");
    bench("1 sector", 1, 1, 100 * SBPAGES + 7);
    bench("8 sectors, contiguous", 8, 1, 110 * SBPAGES + 7);
    bench("512 sectors, contiguous (1 MiB)", 512, 1, 120 * SBPAGES);
    bench("1024 sectors = one whole block (2 MiB)", 1024, 1, 130 * SBPAGES);
    bench("2 sectors, 2 blocks apart", 2, SBPAGES, 140 * SBPAGES + 3);
    bench("8 sectors, 1 per block", 8, SBPAGES, 150 * SBPAGES + 3);
    /* What the storage layer actually does: absorb writes in the overlay,
     * push them into log blocks when it fills, and commit only at shutdown. */
    {
        uint32_t r, blocks[8] = { 60, 61, 75, 90, 91, 120, 121, 140 };

        mock_reset_stats();
        for (r = 0; r < 40; r++)
        {
            nlpn = 1;
            lpns[0] = blocks[r % 8] * SBPAGES + 3 + r;
            if (ftl_hook_write_merge(lpns[0], lpns[0], src, NULL))
                break;
        }
        printf("\n  40 scattered 1-sector writes, no sync yet:\n"
               "    %lu pages written, %lu block erases\n",
               mock_writes, mock_erases);
        ftl_sync();
        printf("  then one sync (folds the logs):\n"
               "    %lu pages written, %lu block erases in total, %.0fx\n",
               mock_writes, mock_erases, (double)mock_writes / 40);
    }

    printf("\none superblock = %d pages = %d KiB; erases are physical blocks,\n",
           SBPAGES, SBPAGES * NAND_PAGE_SIZE / 1024);
    printf("8 per superblock (4 banks x 2 planes)\n");
    return 0;
}
