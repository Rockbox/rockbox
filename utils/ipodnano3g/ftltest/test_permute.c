/* Tests for mock_open_layout(): the dump presented as a mode 9 ("halves")
 * and a mode 4 ("both") chip. Every check compares what the permuted mock
 * returns with an untouched read-only mapping of the dump files, through
 * block translations written out here again from the layout definitions
 * rather than taken from the mock.
 *
 * Usage: test_permute DUMPDIR
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "nand-target.h"
#include "mock_nand.h"

#define DBANKS      4
#define DBLOCKS     4096
#define PPB         128
#define PS          2048
#define METASZ      16
#define DPAGES      ((size_t)DBANKS * DBLOCKS * PPB)

#define CXT_SIZE    0x800
#define CXT_USED    0x1c
#define CXT_REMAP   0x2c
#define CXT_REMAP_N 820
#define CXT_RING    0x694
#define CXT_CKSUM   0x7f8
#define SPARES      89
#define FREE        0xfff0
#define BAD         0xffff

static int fails;
static const uint8_t *raw_data, *raw_meta;
static int both;                 /* the layout under test */
static uint32_t nbanks, nblocks, nunits;

static void check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        fails++;
}

static const uint8_t *map_ro(const char *dir, const char *name, size_t want)
{
    char path[512];
    struct stat st;
    void *p;
    int fd;

    snprintf(path, sizeof(path), "%s/%s", dir, name);
    fd = open(path, O_RDONLY);
    if (fd < 0 || fstat(fd, &st) || (size_t)st.st_size != want)
    {
        fprintf(stderr, "%s: missing or wrong size\n", path);
        exit(2);
    }
    p = mmap(NULL, want, PROT_READ, MAP_SHARED | MAP_NORESERVE, fd, 0);
    if (p == MAP_FAILED)
    {
        perror("mmap");
        exit(2);
    }
    close(fd);
    return p;
}

static uint32_t rng = 0x3a3a1234;
static uint32_t rnd(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

/* ---- The layouts, from their definitions ---- */

/* The formulas' blocks 1 and 2048 (halves) or 4096 (both) trade places, so
 * that the dump's context block 1 of the primary bank is new block 1 */
static uint32_t swap_cxt(uint32_t block)
{
    uint32_t other = both ? 4096 : 2048;

    return block == 1 ? other : block == other ? 1 : block;
}

/* The formulas alone: where the FTL computes a block, as remap entries
 * name replaced blocks */
static void formula_to_dump(uint32_t bank, uint32_t block, uint32_t *db,
                            uint32_t *dk)
{
    uint32_t u, v, hi, r;

    if (!both)
    {
        u = block >= 2048;
        v = block - u * 2048;
        *db = bank;
        *dk = 2 * v + u;
    }
    else
    {
        hi = block >= 4096;
        r = block - hi * 4096;
        v = r / 2;
        u = (r & 1) + 2 * hi;
        *db = 2 * (u % 2) + bank;
        *dk = 2 * v + u / 2;
    }
}

static void formula_from_dump(uint32_t db, uint32_t dk, uint32_t *bank,
                              uint32_t *block)
{
    uint32_t v = dk / 2, plane = dk % 2, u;

    if (!both)
    {
        *bank = db;
        *block = v + plane * 2048;
    }
    else
    {
        u = 2 * plane + db / 2;
        *bank = db % 2;
        *block = 2 * v + (u & 1) + ((u & 2) ? 4096 : 0);
    }
}

/* Where pages physically are: the formulas with the swap */
static void to_dump(uint32_t bank, uint32_t block, uint32_t *db, uint32_t *dk)
{
    formula_to_dump(bank, swap_cxt(block), db, dk);
}

static void from_dump(uint32_t db, uint32_t dk, uint32_t *bank, uint32_t *block)
{
    formula_from_dump(db, dk, bank, block);
    *block = swap_cxt(*block);
}

/* ---- Pages and contexts ---- */

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

static void put32(uint8_t *p, size_t off, uint32_t v)
{
    put16(p, off, v);
    put16(p, off + 2, v >> 16);
}

static void cxt_sums(const uint8_t *c, uint32_t *sum, uint32_t *x)
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

    cxt_sums(c, &sum, &x);
    return sum == get32(c, CXT_CKSUM) && x == get32(c, CXT_CKSUM + 4);
}

static void cxt_seal(uint8_t *c)
{
    uint32_t sum, x;

    cxt_sums(c, &sum, &x);
    put32(c, CXT_CKSUM, sum);
    put32(c, CXT_CKSUM + 4, x);
}

static int is_cxt(const uint32_t *meta, int rc)
{
    return rc > 0 && rc != NAND_ECC_FAILED
           && ((meta[2] >> 8) & 0xff) == 0x80 && (meta[2] & 0xff) == 0;
}

typedef int (*reader)(uint32_t bank, uint32_t page, uint8_t *buf,
                      uint32_t *meta);

/* A page of the dump files, as the NAND driver would read it */
static int raw_read(uint32_t bank, uint32_t page, uint8_t *buf, uint32_t *meta)
{
    size_t g = (size_t)bank * DBLOCKS * PPB + page;
    int32_t rc;

    memcpy(&rc, raw_meta + g * METASZ + 12, 4);
    if (rc == NAND_ECC_CLEAN)
    {
        memset(buf, 0xff, PS);
        memset(meta, 0xff, 12);
        return rc;
    }
    memcpy(buf, raw_data + g * PS, PS);
    memcpy(meta, raw_meta + g * METASZ, 12);
    return rc;
}

static int mock_read(uint32_t bank, uint32_t page, uint8_t *buf, uint32_t *meta)
{
    return nand_read_page(bank, page, buf, meta);
}

/* ftl_vfl_read_cxt(): the first good copy of a commit, into buf */
static int read_cxt(reader rd, uint32_t bank, uint32_t block, uint32_t page,
                    uint8_t *buf, uint32_t *meta)
{
    uint32_t i;

    for (i = page; i < page + 8 && i < PPB; i++)
        if (is_cxt(meta, rd(bank, block * PPB + i, buf, meta)) && cxt_ok(buf))
            return 0;
    return -1;
}

/* The first block from 1 below limit with a context at page 0, or -1 */
static long scan_cxt(reader rd, uint32_t bank, uint32_t limit, uint8_t *buf)
{
    uint32_t meta[3], block;

    for (block = 1; block < limit; block++)
        if (!read_cxt(rd, bank, block, 0, buf, meta))
            return block;
    return -1;
}

/* ftl_vfl_open_bank() on a bank of blocks, with the ring taken from the
 * first context below block limit, into out. Returns 0 or -1. */
static int newest_cxt(reader rd, uint32_t bank, uint32_t limit,
                      uint32_t blocks, uint8_t *out)
{
    static uint8_t buf[PS];
    uint32_t meta[3], ring[4], i, best = 4, bestusn = 0xffffffff, last;

    if (scan_cxt(rd, bank, limit, buf) < 0)
        return -1;
    for (i = 0; i < 4; i++)
        ring[i] = get16(buf, CXT_RING + 2 * i);
    for (i = 0; i < 4; i++)
    {
        if (ring[i] >= blocks || read_cxt(rd, bank, ring[i], 0, buf, meta))
            continue;
        if (meta[0] && meta[0] <= bestusn)
        {
            bestusn = meta[0];
            best = i;
        }
    }
    if (best == 4)
        return -1;
    last = 0;
    for (i = 8; i < PPB; i += 8)
    {
        if (read_cxt(rd, bank, ring[best], i, buf, meta))
            break;
        last = i;
    }
    if (read_cxt(rd, bank, ring[best], last, buf, meta))
        return -1;
    memcpy(out, buf, CXT_SIZE);
    return 0;
}

static uint8_t dump_newest[DBANKS][CXT_SIZE];

/* A remap entry: by the formula, free and bad slots kept */
static uint16_t map_entry(uint32_t db, uint16_t e)
{
    uint32_t bank, block;

    if (e == FREE || e == BAD)
        return e;
    formula_from_dump(db, e, &bank, &block);
    return block;
}

/* A ring block: where it now sits */
static uint16_t map_ring(uint32_t db, uint16_t e)
{
    uint32_t bank, block;

    from_dump(db, e, &bank, &block);
    return block;
}

/* The dump bank a new bank's context header comes from: itself in halves;
 * in both, whichever of ce and ce + 2 has the higher usn, as the FTL takes
 * its control blocks from the highest usn */
static uint32_t header_bank(uint32_t bank)
{
    if (!both)
        return bank;
    return get32(dump_newest[bank + 2], 0) > get32(dump_newest[bank], 0)
           ? bank + 2 : bank;
}

/* What a dump context page should read as: halves rewrites each page */
static void expect_halves(uint32_t db, const uint8_t *src, uint8_t *out)
{
    uint32_t i;

    memcpy(out, src, CXT_SIZE);
    for (i = 0; i < 2 * SPARES; i++)
        put16(out, CXT_REMAP + 2 * i,
              map_entry(db, get16(src, CXT_REMAP + 2 * i)));
    for (i = 0; i < 4; i++)
        if (get16(src, CXT_RING + 2 * i) < DBLOCKS)
            put16(out, CXT_RING + 2 * i,
                  map_ring(db, get16(src, CXT_RING + 2 * i)));
    cxt_seal(out);
}

/* ... and both writes one merge of its two banks' newest into all of them */
static void expect_both(uint32_t ce, uint8_t *out)
{
    uint32_t u, i, db;

    memcpy(out, dump_newest[header_bank(ce)], CXT_SIZE);
    memset(out + CXT_REMAP, 0, 2 * CXT_REMAP_N);
    for (u = 0; u < 4; u++)
    {
        db = 2 * (u % 2) + ce;
        put16(out, CXT_USED + 2 * u,
              get16(dump_newest[db], CXT_USED + 2 * (u / 2)));
        for (i = 0; i < SPARES; i++)
            put16(out, CXT_REMAP + 2 * (u * SPARES + i),
                  map_entry(db, get16(dump_newest[db],
                                      CXT_REMAP + 2 * ((u / 2) * SPARES + i))));
    }
    for (i = 0; i < 4; i++)
    {
        uint16_t e = get16(dump_newest[ce], CXT_RING + 2 * i);
        put16(out, CXT_RING + 2 * i, e < DBLOCKS ? map_ring(ce, e) : e);
    }
    cxt_seal(out);
}

/* ---- The tests ---- */

static void test_geometry(void)
{
    const struct nand_geometry *g = nand_get_geometry();
    char what[128];

    snprintf(what, sizeof(what), "%s: geometry and bank count",
             both ? "both" : "halves");
    check(what, g->banks == nbanks && g->blocks == nblocks
          && g->pagesperblock == PPB && g->planes == nunits
          && g->userblocks == (both ? 7744u : 3872u) && g->vflspares == SPARES
          && !g->twoplane && g->pagesize == PS
          && g->mode == (both ? 4u : 9u)
          && g->layout == (both ? NAND_LAYOUT_BOTH : NAND_LAYOUT_HALVES)
          && g->validated && nand_get_bank_count() == nbanks);
}

/* 2: every dump block is some block of the new layout, exactly once */
static void test_bijection(void)
{
    static uint8_t hits[DBANKS * DBLOCKS];
    uint32_t bank, block, db, dk, db2, dk2, b2, k2;
    unsigned long bad = 0, missed = 0, i;

    memset(hits, 0, sizeof(hits));
    for (bank = 0; bank < nbanks; bank++)
        for (block = 0; block < nblocks; block++)
        {
            to_dump(bank, block, &db, &dk);
            mock_layout_to_dump(bank, block, &db2, &dk2);
            mock_layout_from_dump(db, dk, &b2, &k2);
            if (db >= DBANKS || dk >= DBLOCKS || db != db2 || dk != dk2
                || b2 != bank || k2 != block)
            {
                bad++;
                continue;
            }
            hits[db * DBLOCKS + dk]++;
        }
    for (i = 0; i < DBANKS * DBLOCKS; i++)
        if (hits[i] != 1)
            missed++;
    printf("      %lu mistranslated, %lu dump blocks not hit once\n",
           bad, missed);
    check("translation is a bijection matching the layout definition",
          !bad && !missed);
}

/* 1: random pages read the same as the dump's; context pages excepted */
static void test_random_reads(void)
{
    static uint8_t buf[PS], want[PS];
    uint32_t meta[3], wmeta[3], db, dk, bank, page;
    struct nand_read run[8];
    static uint8_t runbuf[8][PS];
    uint32_t runmeta[8][3];
    unsigned long n = 0, bad = 0, written = 0, cxts = 0;
    int i, j, rc, wrc;

    for (i = 0; i < 4000; i++)
    {
        bank = rnd() % nbanks;
        page = rnd() % (nblocks * PPB);
        to_dump(bank, page / PPB, &db, &dk);
        wrc = raw_read(db, dk * PPB + page % PPB, want, wmeta);
        if (is_cxt(wmeta, wrc))
        {
            cxts++;
            continue;
        }
        rc = nand_read_page(bank, page, buf, meta);
        n++;
        written += wrc != NAND_ECC_CLEAN;
        if (rc != wrc || memcmp(meta, wmeta, sizeof(meta))
            || memcmp(buf, want, PS))
            bad++;
    }
    /* The same through read runs, pages of different banks mixed */
    for (i = 0; i < 125; i++)
    {
        for (j = 0; j < 8; j++)
        {
            run[j].bank = rnd() % nbanks;
            run[j].page = rnd() % (nblocks * PPB);
            run[j].buf = runbuf[j];
            run[j].meta = runmeta[j];
        }
        if (nand_read_pages(run, 8))
        {
            bad++;
            continue;
        }
        for (j = 0; j < 8; j++)
        {
            to_dump(run[j].bank, run[j].page / PPB, &db, &dk);
            wrc = raw_read(db, dk * PPB + run[j].page % PPB, want, wmeta);
            if (is_cxt(wmeta, wrc))
            {
                cxts++;
                continue;
            }
            n++;
            written += wrc != NAND_ECC_CLEAN;
            if (run[j].ecc != wrc || memcmp(runmeta[j], wmeta, sizeof(wmeta))
                || memcmp(runbuf[j], want, PS))
                bad++;
        }
    }
    printf("      %lu pages compared (%lu written), %lu context pages "
           "skipped, %lu differ\n", n, written, cxts, bad);
    check("random pages read as the dump's", !bad && written > 100);
}

/* 1, context pages: every one in the dump, at its new place */
static void test_cxt_pages(void)
{
    static uint8_t buf[PS], want[PS], expect[CXT_SIZE];
    uint32_t meta[3], wmeta[3], db, dk, bank, block;
    unsigned long n = 0, bad = 0, invalid = 0;
    size_t g;
    int rc, wrc;

    for (g = 0; g < DPAGES; g++)
    {
        uint32_t w2;
        int32_t r;

        memcpy(&w2, raw_meta + g * METASZ + 8, 4);
        memcpy(&r, raw_meta + g * METASZ + 12, 4);
        if (((w2 >> 8) & 0xff) != 0x80 || (w2 & 0xff) || r == NAND_ECC_CLEAN)
            continue;
        db = g / ((size_t)DBLOCKS * PPB);
        dk = (g / PPB) % DBLOCKS;
        wrc = raw_read(db, dk * PPB + g % PPB, want, wmeta);
        if (!cxt_ok(want))
        {
            invalid++;
            continue;
        }
        from_dump(db, dk, &bank, &block);
        rc = nand_read_page(bank, block * PPB + g % PPB, buf, meta);
        if (both)
            expect_both(bank, expect);
        else
            expect_halves(db, want, expect);
        n++;
        if (rc != wrc || memcmp(meta, wmeta, sizeof(meta)) || !cxt_ok(buf)
            || memcmp(buf, expect, CXT_SIZE)
            || memcmp(buf + CXT_SIZE, want + CXT_SIZE, PS - CXT_SIZE))
            bad++;
    }
    printf("      %lu context pages, %lu differ from the expected rewrite, "
           "%lu with bad checksums left alone\n", n, bad, invalid);
    check("context pages read as rewritten, spare and tail unchanged",
          n && !bad);
}

/* 3: each bank's context can be found, and its ring holds contexts */
static void test_scan(void)
{
    static uint8_t buf[PS], cxt[PS], rawbuf[PS];
    uint32_t meta[3], bank, i, ring, db, dk;
    long at;
    char what[160];
    int ok, newok, rawok, anyok;

    for (bank = 0; bank < nbanks; bank++)
    {
        at = scan_cxt(mock_read, bank, 200, cxt);
        snprintf(what, sizeof(what),
                 "bank %u: a context in physical blocks 1..199", bank);
        if (at < 0)
        {
            at = scan_cxt(mock_read, bank, nblocks, cxt);
            if (at >= 0)
            {
                to_dump(bank, at, &db, &dk);
                printf("      bank %u: first context at block %ld "
                       "(dump bank %u block %u)\n", bank, at, db, dk);
            }
        }
        else
            printf("      bank %u: first context at block %ld\n", bank, at);
        check(what, at >= 0 && at < 200);
        if (at < 0)
            continue;

        /* The dump's rings list blocks that were never written, so a ring
         * block must hold a context exactly when the block its pages come
         * from does in the dump, and one must */
        ok = 1;
        anyok = 0;
        printf("      ring:");
        for (i = 0; i < 4; i++)
        {
            ring = get16(cxt, CXT_RING + 2 * i);
            if (ring >= nblocks)
            {
                ok = 0;
                continue;
            }
            to_dump(bank, ring, &db, &dk);
            newok = !read_cxt(mock_read, bank, ring, 0, buf, meta);
            rawok = !read_cxt(raw_read, db, dk, 0, rawbuf, meta);
            printf(" %u%s", ring, newok ? "(cxt)" : "");
            ok &= newok == rawok;
            anyok |= newok;
        }
        printf("\n");
        snprintf(what, sizeof(what),
                 "bank %u: ring blocks hold contexts at page 0 as in the dump",
                 bank);
        check(what, ok && anyok);
    }
}

/* 4: the newest context's tables name the same blocks as the dump's */
static void test_remap(void)
{
    static uint8_t cxt[CXT_SIZE];
    uint32_t bank, u, i, sb, splane, db, dk, n, o, hb;
    char what[128];
    unsigned long entries, bad;
    int ok;

    for (bank = 0; bank < nbanks; bank++)
    {
        snprintf(what, sizeof(what),
                 "bank %u: newest context's remap tables and ring", bank);
        if (newest_cxt(mock_read, bank, nblocks, nblocks, cxt))
        {
            check(what, 0);
            continue;
        }
        entries = bad = 0;
        hb = header_bank(bank);
        ok = cxt_ok(cxt)
             && !memcmp(cxt, dump_newest[hb], CXT_USED)
             && !memcmp(cxt + CXT_RING + 8, dump_newest[hb] + CXT_RING + 8,
                        CXT_CKSUM - CXT_RING - 8);
        for (u = 0; u < nunits; u++)
        {
            sb = both ? 2 * (u % 2) + bank : bank;
            splane = both ? u / 2 : u;
            if (get16(cxt, CXT_USED + 2 * u)
                != get16(dump_newest[sb], CXT_USED + 2 * splane))
                ok = 0;
            for (i = 0; i < SPARES; i++)
            {
                o = get16(dump_newest[sb], CXT_REMAP + 2 * (splane * SPARES + i));
                n = get16(cxt, CXT_REMAP + 2 * (u * SPARES + i));
                if (o == FREE || o == BAD)
                {
                    bad += n != o;
                    continue;
                }
                entries++;
                formula_to_dump(bank, n, &db, &dk);
                bad += n >= nblocks || db != sb || dk != o;
            }
        }
        for (i = nunits * SPARES; i < CXT_REMAP_N; i++)
            bad += get16(cxt, CXT_REMAP + 2 * i) != 0;
        for (i = 0; i < 4; i++)
        {
            o = get16(dump_newest[bank], CXT_RING + 2 * i);
            n = get16(cxt, CXT_RING + 2 * i);
            to_dump(bank, n, &db, &dk);
            if (o < DBLOCKS ? (db != bank || dk != o) : n != o)
                ok = 0;
        }
        printf("      bank %u: usn %u, control blocks %u %u %u (header from "
               "dump bank %u), %lu remapped entries, %lu wrong\n",
               bank, get32(cxt, 0), get16(cxt, 4), get16(cxt, 6),
               get16(cxt, 8), hb, entries, bad);
        check(what, ok && !bad);
    }
}

/* 5: erase, program, read a translated block; its neighbours stay */
static int block_is_dump(uint32_t bank, uint32_t block)
{
    static uint8_t buf[PS], want[PS];
    uint32_t meta[3], wmeta[3], db, dk, p;
    int rc, wrc;

    to_dump(bank, block, &db, &dk);
    for (p = 0; p < PPB; p++)
    {
        wrc = raw_read(db, dk * PPB + p, want, wmeta);
        rc = nand_read_page(bank, block * PPB + p, buf, meta);
        if (rc != wrc || memcmp(meta, wmeta, sizeof(meta)))
            return 0;
        if (!is_cxt(wmeta, wrc) && memcmp(buf, want, PS))
            return 0;
    }
    return 1;
}

static void test_erase_program(void)
{
    static uint8_t buf[PS], pat[PS];
    uint32_t meta[3], wmeta[3], bank, block, db, dk, p, i, nb[4], nk[4];
    uint32_t written = 0;
    int ok, rc;
    size_t g;

    /* A written block in the upper half of the new numbering */
    bank = 1;
    block = both ? 4101 : 2100;
    for (; block + 2 < nblocks; block += 2)
    {
        to_dump(bank, block, &db, &dk);
        written = 0;
        for (p = 0; p < PPB; p++)
            written += raw_read(db, dk * PPB + p, buf, meta) != NAND_ECC_CLEAN;
        if (written == PPB)
            break;
    }
    /* Neighbours in the new numbering, and in the dump's */
    nb[0] = nb[1] = bank;
    nk[0] = block - 1;
    nk[1] = block + 1;
    from_dump(db, dk - 1, &nb[2], &nk[2]);
    from_dump(db, dk + 1, &nb[3], &nk[3]);
    printf("      bank %u block %u (dump bank %u block %u), neighbours",
           bank, block, db, dk);
    for (i = 0; i < 4; i++)
        printf(" %u:%u", nb[i], nk[i]);
    printf("\n");

    ok = nand_erase_block(bank, block) == 0;
    for (p = 0; p < PPB && ok; p++)
        ok = nand_read_page(bank, block * PPB + p, buf, meta) == NAND_ECC_CLEAN;
    check("erase of a translated block reads back erased", ok);

    ok = 1;
    for (p = 0; p < 4; p++)
    {
        for (i = 0; i < PS; i++)
            pat[i] = i * 7 + p;
        wmeta[0] = 0x100 + p;
        wmeta[1] = 0xffffffff;
        wmeta[2] = 0xffff4000;
        ok &= nand_write_page(bank, block * PPB + p, pat, wmeta) == 0;
        rc = nand_read_page(bank, block * PPB + p, buf, meta);
        ok &= rc == NAND_ECC_CORRECTED && !memcmp(buf, pat, PS)
              && !memcmp(meta, wmeta, sizeof(meta));
        /* ... and it landed in the dump block the layout says */
        g = ((size_t)db * DBLOCKS + dk) * PPB + p;
        ok &= !memcmp(mock_data + g * PS, pat, PS);
    }
    ok &= !mock_reprogram && !mock_nonsequential;
    check("program of a translated block reads back, in its dump block", ok);

    ok = 1;
    for (i = 0; i < 4; i++)
        ok &= block_is_dump(nb[i], nk[i]);
    check("neighbouring blocks, new and dump numbering, undisturbed", ok);

    /* Bad blocks are named in the new numbering */
    mock_clear_bad();
    mock_set_bad(bank, block, MOCK_FAIL_WRITE | MOCK_FAIL_ERASE);
    ok = nand_erase_block(bank, block) == NAND_OP_FAILED
         && nand_write_page(bank, block * PPB + 4, pat, wmeta)
            == NAND_OP_FAILED;
    ok &= nand_read_page(bank, block * PPB + 3, buf, meta)
          == NAND_ECC_CORRECTED;
    mock_clear_bad();
    mock_set_bad(db, dk, MOCK_FAIL_WRITE);
    ok &= nand_write_page(bank, block * PPB + 4, pat, wmeta) == 0;
    mock_clear_bad();
    check("bad-block injection uses the new numbering", ok);

    /* A torn program lands on the translated page */
    mock_crash_torn = 1;
    mock_crash_after = 1;
    if (!setjmp(mock_crash_jmp))
    {
        nand_write_page(bank, block * PPB + 5, pat, wmeta);
        ok = 0;
    }
    else
        ok = nand_read_page(bank, block * PPB + 5, buf, meta)
                 == NAND_ECC_FAILED
             && nand_read_page(bank, block * PPB + 4, buf, meta)
                 == NAND_ECC_CORRECTED;
    mock_crash_torn = 0;
    mock_crash_after = 0;
    for (i = 0; i < 4; i++)
        ok &= block_is_dump(nb[i], nk[i]);
    check("crash injection cuts the translated page only", ok);
}

static void run(const char *dir, const char *layout)
{
    printf("== %s\n", layout);
    both = !strcmp(layout, "both");
    nbanks = both ? 2 : 4;
    nblocks = both ? 8192 : 4096;
    nunits = both ? 4 : 2;
    mock_open_layout(dir, layout);
    test_geometry();
    test_bijection();
    test_random_reads();
    test_cxt_pages();
    test_scan();
    test_remap();
    test_erase_program();
}

int main(int argc, char **argv)
{
    uint32_t b;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s DUMPDIR\n", argv[0]);
        return 2;
    }
    raw_data = map_ro(argv[1], "nand_data_4banks.bin", DPAGES * PS);
    raw_meta = map_ro(argv[1], "meta_all.bin", DPAGES * METASZ);
    for (b = 0; b < DBANKS; b++)
    {
        if (newest_cxt(raw_read, b, 200, DBLOCKS, dump_newest[b]))
        {
            fprintf(stderr, "dump bank %u: no VFL context\n", b);
            return 2;
        }
        printf("dump bank %u: newest context usn %u\n", b,
               get32(dump_newest[b], 0));
    }

    run(argv[1], "halves");
    run(argv[1], "both");

    printf("%s\n", fails ? "SOME FAILED" : "ALL PASS");
    return fails ? 1 : 0;
}
