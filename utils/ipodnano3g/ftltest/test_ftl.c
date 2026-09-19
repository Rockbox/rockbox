/* Host test of ftl-nano3g.c against the raw dump of a 4GB Nano 3G, or any
 * medium ftl_hook_open() can present (the dump in another layout, or data
 * collected from a contributor's unit).
 *
 * The oracle uses nothing the FTL decodes (no map, no contexts, no VFL
 * tables): for every logical page it takes, among all pages whose spare
 * says "user data, this lpn", the newest - highest usn, then the latest
 * position in its superblock. Every lpn the FTL resolves must land on the
 * oracle's copy (or, where copies tie, on one with identical data).
 *
 * Known false positive: when two logs for the same logical block each hold
 * pages the other lacks, _FTLRestore's own tie-break (0x806a19c in osos
 * 1.1.3, matched here instruction for instruction) keeps the more complete
 * one whole and drops the other entirely, even if the dropped one is newer
 * for some of its pages. ftl_read() then disagrees with this oracle on
 * exactly those pages - correctly, since that is what Apple's own firmware
 * would also resolve to on the same medium. A handful of disagreements
 * confined to one or two logical blocks on a real contributor's dump is
 * this, not a bug; wholesale disagreement is not.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "nand-target.h"
#include "ftl-target.h"
#include "mock_nand.h"

#define BANKS   mock_geo.banks
#define BLOCKS  mock_geo.blocks
#define PPB     mock_geo.pagesperblock
#define PAGE    mock_geo.pagesize
#define NPAGES  ((size_t)BANKS * BLOCKS * PPB)

static uint8_t pagebuf[NAND_MAX_PAGE_SIZE], otherbuf[NAND_MAX_PAGE_SIZE];

static void read_g(size_t g, uint8_t *buf)
{
    nand_read_page(g / ((size_t)BLOCKS * PPB), g % ((size_t)BLOCKS * PPB),
                   buf, NULL);
}

static int fails;

static void check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        fails++;
}

static uint32_t meta_rec[NAND_META_WORDS];

static uint32_t meta_word(size_t g, int w)
{
    nand_read_page(g / ((size_t)BLOCKS * PPB), g % ((size_t)BLOCKS * PPB),
                   pagebuf, meta_rec);
    return meta_rec[w];
}

static int meta_type_g(size_t g)
{
    return (meta_word(g, 2) >> 8) & 0xff;
}

/* Position of a physical page in its superblock's write order */
static uint32_t v_of(size_t g)
{
    uint32_t bank = g / ((size_t)BLOCKS * PPB);
    uint32_t block = (g / PPB) % BLOCKS;
    uint32_t page = g % PPB;
    return page * BANKS * mock_geo.planes
         + ftl_hook_block_unit(block) * BANKS + bank;
}

int main(int argc, char **argv)
{
    const char *spec = argc > 1 ? argv[1] : "../n3g-dump";
    static uint8_t buf[16 * NAND_MAX_PAGE_SIZE] __attribute__((aligned(16)));
    uint32_t nsec, npages, spp, lpn, bank, page;
    unsigned long mount_reads;
    bool realdata = strncmp(spec, "collected:", 10) != 0;
    int rc;

    if (ftl_hook_open(spec))
    {
        puts("cannot open the medium");
        return 2;
    }

    rc = ftl_init();
    mount_reads = mock_reads;
    printf("ftl_init() = %d after %lu page reads\n", rc, mount_reads);
    check("mounts", rc == 0);
    if (rc)
        return 1;
    nsec = ftl_num_sectors();
    spp = PAGE / NAND_PAGE_SIZE;
    npages = nsec / spp;
    check("capacity is the row's user superblocks",
          npages == mock_geo.userblocks / mock_geo.planes * PPB
                    * mock_geo.planes * BANKS);

    /* Landmarks, where the data is real */
    if (realdata && nsec == 1936u * 1024)
    {
        rc = ftl_read(0, 1, buf);
        check("lpn 0 reads and is an MBR (55aa at 0x1fe)",
              rc == 0 && buf[0x1fe] == 0x55 && buf[0x1ff] == 0xaa);
        rc = ftl_read(0x7e, 1, buf);
        check("lpn 0x7e is the firmware partition banner",
              rc == 0 && !memcmp(buf, "{{~~  /-----\\", 13));
    }
    /* Where individual lpns physically live changes whenever Apple's
     * firmware runs, so beyond these landmarks everything is checked
     * against the oracle. */

    /* The oracle */
    uint64_t *best = malloc(npages * sizeof(uint64_t));
    uint32_t *where = malloc(npages * sizeof(uint32_t));
    uint8_t *tie = calloc(npages, 1);
    memset(best, 0, npages * sizeof(uint64_t));
    memset(where, 0xff, npages * sizeof(uint32_t));
    for (size_t g = 0; g < NPAGES; g++)
    {
        int t = meta_type_g(g);
        if (t != 0x40 && t != 0x41)
            continue;
        uint32_t l = meta_rec[0];
        if (l >= npages)
            continue;
        uint64_t key = ((uint64_t)meta_rec[1] << 32) | (v_of(g) + 1);
        if (key > best[l])
        {
            best[l] = key;
            where[l] = g;
            tie[l] = 0;
        }
        else if (key == best[l])
            tie[l] = 1;
    }

    unsigned long agree = 0, tieok = 0, unwritten = 0, bad = 0, shown = 0;
    for (lpn = 0; lpn < npages; lpn++)
    {
        if (ftl_locate(lpn, &bank, &page))
        {
            bad++;
            continue;
        }
        size_t g = (size_t)bank * BLOCKS * PPB + page;
        if (where[lpn] == 0xffffffff)
        {
            /* no copy anywhere: the FTL must point at an erased page */
            if (meta_type_g(g) == 0xff)
                unwritten++;
            else if (bad++, shown++ < 10)
                printf("  lpn %#x: no copy exists, FTL gives %zu (type %#x)\n",
                       lpn, g, meta_type_g(g));
        }
        else if (g == where[lpn])
            agree++;
        else if (tie[lpn] && meta_word(g, 0) == lpn
                 && (memcpy(otherbuf, pagebuf, PAGE),
                     read_g(where[lpn], pagebuf),
                     !memcmp(otherbuf, pagebuf, PAGE)))
            tieok++;
        else if (bad++, shown++ < 10)
            printf("  lpn %#x: FTL gives %zu (usn %#x v %u), oracle %u "
                   "(usn %#x v %u)\n", lpn, g, meta_word(g, 1), v_of(g),
                   where[lpn], meta_word(where[lpn], 1), v_of(where[lpn]));
    }
    printf("oracle: %lu agree, %lu on an identical tied copy, "
           "%lu unwritten, %lu disagree\n", agree, tieok, unwritten, bad);
    check("every lpn resolves to the oracle's newest copy", bad == 0);

    /* Bulk reads through ftl_read, data compared with the oracle's copy */
    unsigned long rerr = 0, derr = 0;
    for (lpn = 0; lpn < npages; lpn += 16)
    {
        uint32_t n = npages - lpn < 16 ? npages - lpn : 16;
        if (ftl_read(lpn * spp, n * spp, buf))
        {
            rerr++;
            continue;
        }
        for (uint32_t i = 0; i < n; i++)
            if (where[lpn + i] != 0xffffffff
                && (read_g(where[lpn + i], pagebuf),
                    memcmp(buf + i * PAGE, pagebuf, PAGE)))
                derr++;
    }
    printf("ftl_read over all %u sectors: %lu failed calls, %lu pages differ\n",
           nsec, rerr, derr);
    check("ftl_read returns the oracle's data for every sector",
          rerr == 0 && derr == 0);
    check("read past the end fails", ftl_read(nsec, 1, buf) != 0);

    printf(fails ? "SOME CHECKS FAILED\n" : "ALL PASS\n");
    return fails ? 1 : 0;
}
