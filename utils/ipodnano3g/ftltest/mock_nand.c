/* Host mock of the Nano 3G NAND driver, backed by the raw dumps:
 *   n3g-dump/nand_data_4banks.bin  page data, bank by bank
 *   n3g-dump/meta_all.bin          16 bytes per page: spare words 0..2 and
 *                                  the nand_read_page() result
 *
 * Both are mapped MAP_PRIVATE, so erase and program are real - they change
 * what later reads return - but they are copy-on-write in RAM and never
 * touch the files. The 4GB mapping costs only the pages actually written.
 *
 * The NAND rules modelled here, measured on the device over a whole 4GB
 * unit (see ../RESULTS.md): programming only ever clears bits; an erased page
 * reads all-0xff with all-0xff metadata and result NAND_ECC_CLEAN; every
 * written page reads NAND_ECC_CORRECTED.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "nand-target.h"
#include "mock_nand.h"

/* The geometry the FTL sees: the dump's, unless mock_open_blank() or
 * mock_open_layout() set another */
static uint32_t BANKS = 4, BLOCKS = 4096, PPB = 128, PAGESIZE = 2048;
#define NPAGES  ((size_t)BANKS * BLOCKS * PPB)
#define METASZ  16

/* Address translation from the numbering the FTL uses onto the backing
 * store. The store is laid out bank-major in SBLOCKS blocks per bank; for
 * mock_open() and mock_open_blank() that is the identity, and for
 * mock_open_layout() the store is the dump's mode 8 layout while the FTL
 * sees another (see mock_layout_to_dump()). */
#define XL_IDENTITY 0
#define XL_HALVES   1
#define XL_BOTH     2
static int xlayout = XL_IDENTITY;
static uint32_t SBLOCKS = 4096;
#define DUMP_BANKS  4
#define DUMP_BLOCKS 4096

static size_t store_page(uint32_t bank, uint32_t page);

uint8_t *mock_data;
uint8_t *mock_meta;
unsigned long mock_reads;
unsigned long mock_read_runs, mock_read_run_pages;
unsigned long mock_writes;
unsigned long mock_writes_by_type[256];
unsigned long mock_crash_after;
jmp_buf mock_crash_jmp;

void *mock_crash_bt[32];
int mock_crash_btn;

int mock_crash_torn;

static void set_rc(size_t g, int32_t rc);

/* g: the page (program) or first page of the block (erase), is_erase */
static void crash_point(size_t g, int is_erase)
{
    if (mock_crash_after && !--mock_crash_after)
    {
        mock_crash_btn = backtrace(mock_crash_bt, 32);
        if (mock_crash_torn && !is_erase)
        {
            /* A program cut short: garbage that fails ECC */
            memset(mock_data + g * PAGESIZE, 0x5a, PAGESIZE);
            memset(mock_meta + g * METASZ, 0x00, NAND_META_WORDS * 4);
            set_rc(g, NAND_ECC_FAILED);
        }
        else if (mock_crash_torn && is_erase)
        {
            /* An erase cut short: the first half of the block erased, the
             * rest unreadable */
            uint32_t p;
            for (p = 0; p < PPB; p++)
            {
                if (p < PPB / 2)
                {
                    memset(mock_data + (g + p) * PAGESIZE, 0xff, PAGESIZE);
                    memset(mock_meta + (g + p) * METASZ, 0xff, NAND_META_WORDS * 4);
                    set_rc(g + p, NAND_ECC_CLEAN);
                }
                else
                    set_rc(g + p, NAND_ECC_FAILED);
            }
        }
        longjmp(mock_crash_jmp, 1);
    }
}
unsigned long mock_erases;
unsigned long mock_reprogram;
unsigned long mock_nonsequential;

struct nand_geometry mock_geo = {
    .banks = 4, .blocks = 4096, .pagesperblock = 128, .planes = 2,
    .userblocks = 3872, .vflspares = 89, .twoplane = true, .pagesize = 2048,
    .mode = 8, .layout = NAND_LAYOUT_ADJACENT, .validated = true,
};

unsigned long mock_runs, mock_run_pages, mock_twoplane_runs, mock_twoplane_pages;
unsigned long mock_bad_twoplane;

/* Injected bad blocks */
#define MAX_BAD 32
static struct { uint32_t bank, block; int how; } bad[MAX_BAD];
static unsigned int nbad;
unsigned long mock_write_fails, mock_erase_fails;

void mock_set_bad(uint32_t bank, uint32_t block, int how)
{
    if (nbad < MAX_BAD)
    {
        bad[nbad].bank = bank;
        bad[nbad].block = block;
        bad[nbad].how = how;
        nbad++;
    }
}

void mock_clear_bad(void)
{
    nbad = 0;
    mock_write_fails = mock_erase_fails = 0;
}

static int is_bad(uint32_t bank, uint32_t block, int how)
{
    unsigned int i;

    for (i = 0; i < nbad; i++)
        if (bad[i].bank == bank && bad[i].block == block
            && (bad[i].how & how))
            return 1;
    return 0;
}

static uint8_t *map_file(const char *path, size_t want)
{
    int fd = open(path, O_RDONLY);
    struct stat st;
    void *p;

    if (fd < 0 || fstat(fd, &st) || (size_t)st.st_size != want)
    {
        fprintf(stderr, "%s: missing or wrong size\n", path);
        exit(2);
    }
    /* MAP_PRIVATE + PROT_WRITE: writes are copy-on-write, the file is
     * never modified. MAP_NORESERVE keeps the 4GB mapping from being
     * charged against commit limits up front. */
    p = mmap(NULL, want, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_NORESERVE, fd, 0);
    if (p == MAP_FAILED)
    {
        perror("mmap");
        exit(2);
    }
    close(fd);
    return p;
}

/* A blank medium of any geometry: every page erased. Anonymous zero pages
 * read as erased (see nand_read_page), so only written pages cost memory. */
void mock_open_blank(void)
{
    BANKS = mock_geo.banks;
    BLOCKS = mock_geo.blocks;
    PPB = mock_geo.pagesperblock;
    PAGESIZE = mock_geo.pagesize;
    xlayout = XL_IDENTITY;
    SBLOCKS = BLOCKS;
    mock_data = mmap(NULL, NPAGES * PAGESIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    mock_meta = mmap(NULL, NPAGES * METASZ, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (mock_data == MAP_FAILED || mock_meta == MAP_FAILED)
    {
        perror("mmap");
        exit(2);
    }
    mock_reset_stats();
}

void mock_open(const char *dir)
{
    char path[512];

    xlayout = XL_IDENTITY;
    SBLOCKS = BLOCKS;

    snprintf(path, sizeof(path), "%s/nand_data_4banks.bin", dir);
    mock_data = map_file(path, NPAGES * PAGESIZE);
    snprintf(path, sizeof(path), "%s/meta_all.bin", dir);
    mock_meta = map_file(path, NPAGES * METASZ);
    mock_reset_stats();
}

void mock_reset_stats(void)
{
    mock_reads = mock_writes = mock_erases = 0;
    mock_read_runs = mock_read_run_pages = 0;
    mock_reprogram = mock_nonsequential = 0;
    mock_runs = mock_run_pages = mock_twoplane_runs = mock_twoplane_pages = 0;
    mock_bad_twoplane = 0;
    memset(mock_writes_by_type, 0, sizeof(mock_writes_by_type));
}

static int32_t page_rc(size_t g)
{
    int32_t rc;
    memcpy(&rc, mock_meta + g * METASZ + 12, 4);
    return rc;
}

static void set_rc(size_t g, int32_t rc)
{
    memcpy(mock_meta + g * METASZ + 12, &rc, 4);
}

/* An erased page is the one that reads NAND_ECC_CLEAN; every written page
 * reads NAND_ECC_CORRECTED. */
static int is_erased(size_t g)
{
    return page_rc(g) == NAND_ECC_CLEAN;
}

int nand_read_page(uint32_t bank, uint32_t page, void *databuf,
                   uint32_t *meta)
{
    size_t g;

    if (bank >= BANKS || page >= BLOCKS * PPB)
        return -1;
    g = store_page(bank, page);
    mock_reads++;
    /* An erased page reads all 0xff, as every one in the dump does; a blank
     * medium's untouched pages are zero-filled and read the same way */
    if (is_erased(g))
    {
        memset(databuf, 0xff, PAGESIZE);
        if (meta)
            memset(meta, 0xff, NAND_META_WORDS * 4);
        return NAND_ECC_CLEAN;
    }
    memcpy(databuf, mock_data + g * PAGESIZE, PAGESIZE);
    if (meta)
        memcpy(meta, mock_meta + g * METASZ, NAND_META_WORDS * 4);
    return page_rc(g);
}

int nand_read_pages(struct nand_read *r, unsigned int n)
{
    unsigned int i;

    if (!n)
        return -1;
    mock_read_runs++;
    mock_read_run_pages += n;
    for (i = 0; i < n; i++)
    {
        r[i].ecc = nand_read_page(r[i].bank, r[i].page, r[i].buf, r[i].meta);
        if (r[i].ecc < 0)
            return -1;
    }
    return 0;
}

int nand_write_page(uint32_t bank, uint32_t page, const void *databuf,
                    const uint32_t *meta)
{
    size_t g;
    const uint8_t *src = databuf;
    uint8_t *dst;
    uint32_t i;

    if (bank >= BANKS || page >= BLOCKS * PPB)
        return -1;
    g = store_page(bank, page);
    crash_point(g, 0);
    if (is_bad(bank, page / PPB, MOCK_FAIL_WRITE))
    {
        mock_write_fails++;
        return NAND_OP_FAILED;
    }
    mock_writes++;
    mock_writes_by_type[(meta[2] >> 8) & 0xff]++;

    /* Programming a page that is not erased is undefined on real NAND;
     * the FTL must never do it. Count it, then model what the chip does. */
    if (!is_erased(g))
        mock_reprogram++;
    /* Pages within a block must be programmed in order on MLC. */
    if ((page % PPB) && is_erased(g - 1))
        mock_nonsequential++;

    /* Program only ever clears bits, from all 0xff if erased */
    if (is_erased(g))
    {
        memset(mock_data + g * PAGESIZE, 0xff, PAGESIZE);
        memset(mock_meta + g * METASZ, 0xff, NAND_META_WORDS * 4);
    }
    dst = mock_data + g * PAGESIZE;
    for (i = 0; i < PAGESIZE; i++)
        dst[i] &= src[i];
    dst = mock_meta + g * METASZ;
    for (i = 0; i < NAND_META_WORDS * 4; i++)
        dst[i] &= ((const uint8_t *)meta)[i];
    set_rc(g, NAND_ECC_CORRECTED);
    return 0;
}

/* A run is its pages one after another, as far as the medium can tell. A
 * failing page stops the run there, which is one of the outcomes the driver
 * allows ("any page of the run may not have been written"). Two-plane runs
 * are checked for pairs the chip could actually take: plane 0 then plane 1
 * of the same bank, the same page, in a block and its twin. */
int nand_write_pages(const struct nand_write *w, unsigned int n,
                     bool two_plane, uint32_t *failbank)
{
    unsigned int i;
    int rc;

    if (!n || (two_plane && (n & 1)))
        return -1;
    mock_runs++;
    mock_run_pages += n;
    if (two_plane)
    {
        mock_twoplane_runs++;
        mock_twoplane_pages += n;
        for (i = 0; i < n; i += 2)
            if (w[i].bank != w[i + 1].bank
                || (w[i].page / PPB) % 2 != 0
                || w[i + 1].page != w[i].page + PPB)
                mock_bad_twoplane++;
    }
    for (i = 0; i < n; i++)
    {
        rc = nand_write_page(w[i].bank, w[i].page, w[i].buf, w[i].meta);
        if (rc)
        {
            if (failbank)
                *failbank = w[i].bank;
            return rc;
        }
    }
    return 0;
}

int nand_erase_block(uint32_t bank, uint32_t block)
{
    size_t g;
    uint32_t p;

    if (bank >= BANKS || block >= BLOCKS)
        return -1;
    g = store_page(bank, block * PPB);
    crash_point(g, 1);
    if (is_bad(bank, block, MOCK_FAIL_ERASE))
    {
        mock_erase_fails++;
        return NAND_OP_FAILED;
    }
    mock_erases++;
    memset(mock_data + g * PAGESIZE, 0xff, (size_t)PPB * PAGESIZE);
    for (p = 0; p < PPB; p++)
    {
        memset(mock_meta + (g + p) * METASZ, 0xff, NAND_META_WORDS * 4);
        set_rc(g + p, NAND_ECC_CLEAN);
    }
    return 0;
}

int mock_snapshot(const char *dir)
{
    char path[512];
    int fd;

    snprintf(path, sizeof(path), "%s/nand_data_4banks.bin", dir);
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return -1;
    if (write(fd, mock_data, NPAGES * PAGESIZE)
        != (ssize_t)(NPAGES * PAGESIZE))
    {
        close(fd);
        return -1;
    }
    close(fd);

    snprintf(path, sizeof(path), "%s/meta_all.bin", dir);
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return -1;
    if (write(fd, mock_meta, NPAGES * METASZ) != (ssize_t)(NPAGES * METASZ))
    {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

unsigned int nand_get_bank_count(void)
{
    return BANKS;
}

const struct nand_geometry *nand_get_geometry(void)
{
    return &mock_geo;
}

/* ---- Other layouts over the dump ----
 *
 * The dump's chip is mode 8: VFL block v of unit u is physical block 2v+u,
 * four banks of 4096 blocks. mock_open_layout() presents the same pages as a
 * chip of another layout would hold them - each superblock's pages in the
 * same order - by translating block numbers on every access. Only the VFL
 * contexts must change, because they hold physical block numbers; they are
 * rewritten in the copy-on-write mapping when the layout is opened. */

/* Apple formats its context ring within physical blocks 1..199, but the
 * layout formulas put the dump's context block (block 1 of each bank) at
 * 2048 or 4096. So each new bank swaps two blocks of the formula's
 * numbering: new block 1 becomes the primary dump bank's block 1, and the
 * block the formula put at 1 (halves: the same bank's block 2; both: the
 * secondary bank's block 0) takes block 1's formula place. Dump blocks 0-4
 * are all remapped by the VFL, so no FTL data moves. */
static uint32_t swap_cxt_block(uint32_t block)
{
    uint32_t other = xlayout == XL_HALVES ? DUMP_BLOCKS / 2 : DUMP_BLOCKS;

    if (xlayout == XL_IDENTITY)
        return block;
    return block == 1 ? other : block == other ? 1 : block;
}

void mock_layout_to_dump(uint32_t bank, uint32_t block,
                         uint32_t *dbank, uint32_t *dblock)
{
    uint32_t u, v, hi, r;

    block = swap_cxt_block(block);
    switch (xlayout)
    {
    case XL_HALVES:
        u = block >= DUMP_BLOCKS / 2;
        v = block - u * (DUMP_BLOCKS / 2);
        *dbank = bank;
        *dblock = 2 * v + u;
        break;
    case XL_BOTH:
        hi = block >= DUMP_BLOCKS;
        r = block - hi * DUMP_BLOCKS;
        v = r / 2;
        u = (r & 1) + 2 * hi;
        *dbank = 2 * (u % 2) + bank;
        *dblock = 2 * v + u / 2;
        break;
    default:
        *dbank = bank;
        *dblock = block;
        break;
    }
}

/* The layout formula alone, without the context block swap: where the FTL
 * computes a block to be. A VFL remap entry names a replaced block this way,
 * whatever physically holds its (never read) contents. */
static void formula_from_dump(uint32_t dbank, uint32_t dblock,
                              uint32_t *bank, uint32_t *block)
{
    uint32_t u, v = dblock / 2, plane = dblock % 2;

    switch (xlayout)
    {
    case XL_HALVES:
        *bank = dbank;
        *block = v + plane * (DUMP_BLOCKS / 2);
        break;
    case XL_BOTH:
        u = 2 * plane + dbank / 2;
        *bank = dbank % 2;
        *block = 2 * v + (u & 1) + ((u & 2) ? DUMP_BLOCKS : 0);
        break;
    default:
        *bank = dbank;
        *block = dblock;
        break;
    }
}

void mock_layout_from_dump(uint32_t dbank, uint32_t dblock,
                           uint32_t *bank, uint32_t *block)
{
    formula_from_dump(dbank, dblock, bank, block);
    *block = swap_cxt_block(*block);
}

/* The store index of a page in the numbering the FTL sees */
static size_t store_page(uint32_t bank, uint32_t page)
{
    uint32_t dbank, dblock;

    if (xlayout == XL_IDENTITY)
        return (size_t)bank * BLOCKS * PPB + page;
    mock_layout_to_dump(bank, page / PPB, &dbank, &dblock);
    return ((size_t)dbank * SBLOCKS + dblock) * PPB + page % PPB;
}

#define CXT_SIZE        0x800
#define CXT_USEDCOUNT   0x1c
#define CXT_REMAP       0x2c
#define CXT_REMAP_N     820
#define CXT_RING        0x694
#define CXT_CKSUM       0x7f8
#define VFL_SPARES      89
#define VFL_FREE        0xfff0
#define VFL_BAD         0xffff

static uint16_t get16(const uint8_t *p, size_t off)
{
    return p[off] | p[off + 1] << 8;
}

static void put16(uint8_t *p, size_t off, uint16_t v)
{
    p[off] = v;
    p[off + 1] = v >> 8;
}

static uint32_t get32(const uint8_t *p, size_t off)
{
    return get16(p, off) | (uint32_t)get16(p, off + 2) << 16;
}

static void cxt_checksum(const uint8_t *c, uint32_t *sum, uint32_t *x)
{
    size_t off;

    *sum = *x = 0xAABBCCDD;
    for (off = 0; off < CXT_CKSUM; off += 4)
    {
        *sum += get32(c, off);
        *x ^= get32(c, off);
    }
}

static int cxt_ok(const uint8_t *c)
{
    uint32_t sum, x;

    cxt_checksum(c, &sum, &x);
    return sum == get32(c, CXT_CKSUM) && x == get32(c, CXT_CKSUM + 4);
}

static void cxt_seal(uint8_t *c)
{
    uint32_t sum, x;

    cxt_checksum(c, &sum, &x);
    put16(c, CXT_CKSUM, sum);
    put16(c, CXT_CKSUM + 2, sum >> 16);
    put16(c, CXT_CKSUM + 4, x);
    put16(c, CXT_CKSUM + 6, x >> 16);
}

static uint32_t store_word(size_t g, int w)
{
    uint32_t v;
    memcpy(&v, mock_meta + g * METASZ + 4 * w, 4);
    return v;
}

/* Store page g holds a VFL context with good checksums, as the FTL's
 * ftl_vfl_read_cxt() accepts one */
static int store_is_cxt(size_t g)
{
    uint32_t w2 = store_word(g, 2);

    return !is_erased(g) && page_rc(g) >= 0 && page_rc(g) != NAND_ECC_FAILED
           && ((w2 >> 8) & 0xff) == 0x80 && (w2 & 0xff) == 0
           && cxt_ok(mock_data + g * PAGESIZE);
}

/* The first good context of the commit at page of a dump block, or -1 */
static long store_read_cxt(uint32_t dbank, uint32_t dblock, uint32_t page)
{
    size_t base = ((size_t)dbank * SBLOCKS + dblock) * PPB;
    uint32_t i;

    for (i = page; i < page + 8 && i < PPB; i++)
        if (store_is_cxt(base + i))
            return base + i;
    return -1;
}

/* The newest VFL context of a dump bank, found as ftl_vfl_open_bank() does */
static const uint8_t *store_newest_cxt(uint32_t dbank)
{
    uint16_t ring[4];
    uint32_t i, block, last, best = 4, bestusn = 0xffffffff, w0;
    long g = -1;

    for (block = 1; block < 200; block++)
        if ((g = store_read_cxt(dbank, block, 0)) >= 0)
            break;
    if (g < 0)
        return NULL;
    for (i = 0; i < 4; i++)
        ring[i] = get16(mock_data + g * PAGESIZE, CXT_RING + 2 * i);
    for (i = 0; i < 4; i++)
    {
        if (ring[i] >= SBLOCKS || (g = store_read_cxt(dbank, ring[i], 0)) < 0)
            continue;
        w0 = store_word(g, 0);
        if (w0 && w0 <= bestusn)
        {
            bestusn = w0;
            best = i;
        }
    }
    if (best == 4)
        return NULL;
    block = ring[best];
    last = 0;
    for (i = 8; i < PPB; i += 8)
    {
        if (store_read_cxt(dbank, block, i) < 0)
            break;
        last = i;
    }
    g = store_read_cxt(dbank, block, last);
    return g < 0 ? NULL : mock_data + g * PAGESIZE;
}

/* A dump bank's block number as the new layout numbers it: where it now
 * sits (for the ring, which is scanned), or with remap where the formula
 * puts it (for a remap entry) */
static uint16_t new_block(uint32_t dbank, uint16_t dblock, int remap)
{
    uint32_t bank, block;

    if (remap)
        formula_from_dump(dbank, dblock, &bank, &block);
    else
        mock_layout_from_dump(dbank, dblock, &bank, &block);
    return block;
}

/* halves: every context keeps its bank, so each page is rewritten alone */
static void rewrite_halves(void)
{
    size_t g, npages = (size_t)DUMP_BANKS * SBLOCKS * PPB;
    uint32_t dbank, i;
    uint8_t *c;
    uint16_t e;

    for (g = 0; g < npages; g++)
    {
        if (!store_is_cxt(g))
            continue;
        dbank = g / ((size_t)SBLOCKS * PPB);
        c = mock_data + g * PAGESIZE;
        for (i = 0; i < 2 * VFL_SPARES; i++)
        {
            e = get16(c, CXT_REMAP + 2 * i);
            if (e != VFL_FREE && e != VFL_BAD)
                put16(c, CXT_REMAP + 2 * i, new_block(dbank, e, 1));
        }
        for (i = 0; i < 4; i++)
        {
            e = get16(c, CXT_RING + 2 * i);
            if (e < SBLOCKS)
                put16(c, CXT_RING + 2 * i, new_block(dbank, e, 0));
        }
        cxt_seal(c);
    }
}

/* both: each new bank takes units 0 and 2 from dump bank ce and units 1 and
 * 3 from dump bank ce + 2. The newest contexts of the two are merged (unit
 * tables from each, header from the higher usn, ring from ce), and the
 * merge replaces every context page of both, so whichever copy a VFL reads -
 * including the secondary bank's, which now sit outside the ring - agrees. */
static void rewrite_both(void)
{
    static uint8_t merged[CXT_SIZE];
    const uint8_t *src[2];
    size_t g, base, bankpages = (size_t)SBLOCKS * PPB;
    uint32_t ce, u, i, dbank, nbank, nblock, side;
    uint16_t e;

    for (ce = 0; ce < 2; ce++)
    {
        src[0] = store_newest_cxt(ce);
        src[1] = store_newest_cxt(ce + 2);
        if (!src[0] || !src[1])
        {
            fprintf(stderr, "mock_open_layout: no VFL context on bank %u\n",
                    src[0] ? ce + 2 : ce);
            exit(2);
        }
        /* The header - usn, FTL control blocks, update count and the rest -
         * from the newer of the two, as the FTL takes its control blocks
         * from the highest usn of any bank */
        memcpy(merged, src[get32(src[1], 0) > get32(src[0], 0)], CXT_SIZE);
        memset(merged + CXT_REMAP, 0, 2 * CXT_REMAP_N);
        for (u = 0; u < 4; u++)
        {
            const uint8_t *s = src[u % 2];
            uint32_t plane = u / 2;

            dbank = 2 * (u % 2) + ce;
            put16(merged, CXT_USEDCOUNT + 2 * u,
                  get16(s, CXT_USEDCOUNT + 2 * plane));
            for (i = 0; i < VFL_SPARES; i++)
            {
                e = get16(s, CXT_REMAP + 2 * (plane * VFL_SPARES + i));
                if (e != VFL_FREE && e != VFL_BAD)
                {
                    formula_from_dump(dbank, e, &nbank, &nblock);
                    e = nblock;
                }
                put16(merged, CXT_REMAP + 2 * (u * VFL_SPARES + i), e);
            }
        }
        for (i = 0; i < 4; i++)
        {
            /* The ring is the primary's, which is where it sits */
            e = get16(src[0], CXT_RING + 2 * i);
            put16(merged, CXT_RING + 2 * i,
                  e < SBLOCKS ? new_block(ce, e, 0) : e);
        }
        cxt_seal(merged);

        for (side = 0; side < 2; side++)
        {
            base = (ce + 2 * side) * bankpages;
            for (g = base; g < base + bankpages; g++)
                if (store_is_cxt(g))
                    memcpy(mock_data + g * PAGESIZE, merged, CXT_SIZE);
        }
    }
}

void mock_open_layout(const char *dir, const char *layout)
{
    int l = !strcmp(layout, "halves") ? XL_HALVES
          : !strcmp(layout, "both") ? XL_BOTH : -1;

    if (l < 0)
    {
        fprintf(stderr, "mock_open_layout: unknown layout %s\n", layout);
        exit(2);
    }
    /* Map and rewrite in the dump's own numbering */
    BANKS = DUMP_BANKS;
    BLOCKS = DUMP_BLOCKS;
    PPB = 128;
    PAGESIZE = 2048;
    mock_open(dir);
    xlayout = l;
    if (l == XL_HALVES)
        rewrite_halves();
    else
        rewrite_both();

    if (l == XL_HALVES)
        mock_geo = (struct nand_geometry){
            .banks = 4, .blocks = 4096, .pagesperblock = 128, .planes = 2,
            .userblocks = 3872, .vflspares = 89, .twoplane = false,
            .pagesize = 2048, .mode = 9, .layout = NAND_LAYOUT_HALVES,
            .validated = true,
        };
    else
        mock_geo = (struct nand_geometry){
            .banks = 2, .blocks = 8192, .pagesperblock = 128, .planes = 4,
            .userblocks = 7744, .vflspares = 89, .twoplane = false,
            .pagesize = 2048, .mode = 4, .layout = NAND_LAYOUT_BOTH,
            .validated = true,
        };
    BANKS = mock_geo.banks;
    BLOCKS = mock_geo.blocks;
}
