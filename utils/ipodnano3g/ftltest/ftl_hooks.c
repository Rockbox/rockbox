/* Query hooks for the host tests. They read the FTL's static state, so this
 * file is appended to ftl-nano3g.c at build time (see the Makefile) rather
 * than compiled on its own. A hook group is only appended when the FTL
 * source lacks it, so this also
 * builds against trees that still define them. */

#include "mock_nand.h"

#ifdef NEED_HOOKS_LOCATE
/* The remap slot whose reserved block is physical block block: what it
 * replaces, or 0xfff0/0xffff; -1 if block is not a reserved block */
int ftl_vfl_pool_entry(uint32_t bank, uint32_t block)
{
    uint32_t k;

    if (!geo || bank >= geo->banks)
        return -1;
    for (k = 0; k < geo->planes * geo->vflspares; k++)
        if (ftl_vfl_spare_block(k) == block)
            return ftl_vfl_cxt[bank].remap[k];
    return -1;
}

int ftl_sb_locate(uint32_t sb, uint32_t v, uint32_t *bank, uint32_t *page)
{
    if (!ftl_mounted || sb >= nsuperblocks || v >= sbpages)
        return -1;
    ftl_vpage_phys(sb, v, bank, page);
    return 0;
}

int ftl_locate(uint32_t lpn, uint32_t *bank, uint32_t *page)
{
    uint32_t sb, v;

    if (!ftl_mounted || ftl_resolve(lpn, &sb, &v))
        return -1;
    ftl_vpage_phys(sb, v, bank, page);
    return 0;
}
#endif

#if defined(NEED_HOOKS_WEAR) && defined(FTL_COMMIT_PAGES) /* needs the write path */
void ftl_global_spread(uint32_t *lowest, uint32_t *highest)
{
    uint32_t i, mn = 0xffff, mx = 0;

    for (i = 0; i < nsuperblocks; i++)
    {
        if (ftl_erasectr[i] == 0xffff)
            continue;
        if (ftl_erasectr[i] < mn) mn = ftl_erasectr[i];
        if (ftl_erasectr[i] > mx) mx = ftl_erasectr[i];
    }
    *lowest = mn;
    *highest = mx;
}

uint32_t ftl_pool_snapshot(uint16_t *sb, uint16_t *erasectr, uint32_t max)
{
    uint32_t i, n = 0;

    if (!ftl_mounted)
        return 0;
    for (i = 0; i < ftl_cxt.freecount && n < max; i++)
    {
        sb[n] = ftl_cxt.blockpool[(ftl_cxt.nextfreeidx + i) % FTL_POOL_SIZE];
        erasectr[n] = sb[n] < nsuperblocks ? ftl_erasectr[sb[n]] : 0xffff;
        n++;
    }
    return n;
}
#endif

/* Pool state behind ftl_pool_alloc()'s choice: blocks free at the last
 * commit are safe to hand out, blocks released since are not */
void ftl_pool_state(uint32_t *freecount, uint32_t *fresh, uint32_t *nlogs)
{
    *freecount = ftl_cxt.freecount;
    *fresh = 0;                         /* no fresh-block rule any more */
    *nlogs = ftl_nlogs;
}

/* ftl_write_merge() and the write state are private to the FTL; the tests
 * reach them through these */
int ftl_hook_write_merge(uint32_t first, uint32_t last,
                         const void *(*src)(uint32_t lpn, void *ctx),
                         void *ctx)
{
    return ftl_write_merge(first, last, src, ctx);
}

bool ftl_hook_writeable(void)
{
    return ftl_mounted && ftl_writable;
}

/* The three FTL control superblocks, so a test can roll them back */
void ftl_hook_ctrl(uint16_t *sb)
{
    unsigned int i;

    for (i = 0; i < 3; i++)
        sb[i] = ftl_ctrl[i];
}

/* Consistency of the mounted FTL's RAM state against itself and the medium.
 * Returns 0, or writes a description of the first problem into msg. */
int ftl_hook_check(char *msg, size_t len)
{
    static uint8_t use[FTL_MAX_SB];     /* 1 map, 2 pool, 3 log, 4 ctrl */
    uint32_t i, v, sb;

    memset(use, 0, sizeof(use));
    for (i = 0; i < 3; i++)
        if (ftl_ctrl[i] < nsuperblocks)
            use[ftl_ctrl[i]] = 4;
    for (i = 0; i < usersb; i++)
    {
        sb = ftl_map[i];
        if (use[sb])
        {
            snprintf(msg, len, "map lblock %u -> sb %u, already used as %u", i, sb, use[sb]);
            return -1;
        }
        use[sb] = 1;
    }
    for (i = 0; i < ftl_nlogs; i++)
    {
        const struct ftl_log *log = &ftl_logs[i];
        sb = log->sb;
        if (use[sb])
        {
            snprintf(msg, len, "log %u (lblock %u) sb %u already used as %u", i, log->lblock, sb, use[sb]);
            return -1;
        }
        use[sb] = 3;
        for (v = 0; v < sbpages; v++)
            if (log->offsets[v] != FTL_NO_PAGE)
            {
                if (ftl_read_vpage(sb, log->offsets[v]) < 0 || !meta_is_data(ftl_meta)
                    || ftl_meta[0] != log->lblock * sbpages + v)
                {
                    snprintf(msg, len, "log lblock %u idx %u -> sb %u v %u holds lpn %x type %x",
                             log->lblock, v, sb, log->offsets[v], ftl_meta[0], meta_type(ftl_meta));
                    return -1;
                }
            }
    }
    if (1)
        for (i = 0; i < ftl_cxt.freecount; i++)
        {
            sb = ftl_cxt.blockpool[(ftl_cxt.nextfreeidx + i) % FTL_POOL_SIZE];
            if (use[sb])
            {
                snprintf(msg, len, "pool sb %u already used as %u", sb, use[sb]);
                return -1;
            }
            use[sb] = 2;
        }
    if (ftl_cxt.freecount + ftl_nlogs > FTL_POOL_SIZE)
    {
        snprintf(msg, len, "free %u + logs %u > 20", ftl_cxt.freecount, ftl_nlogs);
        return -1;
    }
    if (!ftl_writable)
    {
        snprintf(msg, len, "not writable after mount");
        return -1;
    }
    return 0;
}

/* What ftl_load_cxt() sees in the three control blocks */
void ftl_hook_ctrl_summary(void)
{
    uint32_t i, v, last;
    int type, lt;

    for (i = 0; i < 3; i++)
    {
        uint32_t u0 = 0;
        int t0 = ftl_read_vpage(ftl_ctrl[i], 0);
        u0 = ftl_meta[0];
        last = 0; lt = -9;
        for (v = 0; v < sbpages; v++)
        {
            type = ftl_read_vpage(ftl_ctrl[i], v);
            if (type == SPARE_ERASED) break;
            lt = type; last = v;
        }
        printf("     ctrl %u = sb %u: page0 type %x usn %08x, used %u, last type %x usn %08x\n",
               i, ftl_ctrl[i], t0, u0, v, lt, ftl_meta[0]);
        (void)last;
    }
    printf("     ftl_ctrl_idx %u ftl_ctrl_v %u cxt.usn %08x cleanflag %u maxusn %u\n",
           ftl_ctrl_idx, ftl_ctrl_v, ftl_cxt.usn, ftl_cxt.cleanflag, ftl_cxt.maxusn);
}

/* Fold every open log and commit, leaving the medium's data unchanged:
 * sector 0 written back as it is, then a full flush and a remount */
int ftl_hook_settle(void)
{
    static uint8_t s0[NAND_PAGE_SIZE];

    if (ftl_read(0, 1, s0) || ftl_write(0, 1, s0) || ftl_sync())
        return -1;
    return ftl_init();
}

/* Lay down a clean, empty volume on a blank mock medium in Apple's format,
 * for geometries no dump exists of: per bank a VFL context whose ring is
 * blocks 1-4, with the blocks they and block 0 fall in remapped to the
 * first reserved blocks as on the dumped unit; FTL control superblocks 0-2;
 * logical block n in superblock 3 + n; the 20 after them free; then one
 * commit. Every data page reads erased. mock_open_blank() first. */
int ftl_hook_format(void)
{
    uint32_t bank, i, n;

    ftl_mounted = false;
    mutex_init(&ftl_mtx);
    ftl_nholes = 0;
    if (ftl_setup_geometry())
        return -1;
    ftl_vfl_usn = 0;
    for (bank = 0; bank < geo->banks; bank++)
    {
        struct ftl_vfl_cxt *cxt = &ftl_vfl_cxt[bank];
        uint32_t used[4] = { 0, 0, 0, 0 };

        memset(cxt, 0, sizeof(*cxt));
        cxt->updatecount = 0xffffffff;
        for (i = 0; i < 3; i++)
            cxt->ftlctrlblocks[i] = i;
        for (i = 0; i < 4; i++)
            cxt->vflcxtblocks[i] = 1 + i;
        for (i = 0; i < geo->planes * geo->vflspares; i++)
            cxt->remap[i] = FTL_VFL_FREE;
        /* Physical blocks 0-4, each in the table of the unit it belongs to */
        for (i = 0; i <= 4; i++)
        {
            uint32_t unit = 0, v = 0;

            while (ftl_unit_block(unit, v) != i)
                if (++v == 8)
                {
                    v = 0;
                    unit++;
                }
            cxt->remap[unit * geo->vflspares + used[unit]++] = i;
        }
        for (i = 0; i < 4; i++)
            cxt->usedcount[i] = used[i];
        if (ftl_vfl_store_cxt(bank))
            return -2;
    }

    memcpy(ftl_ctrl, ftl_vfl_cxt[0].ftlctrlblocks, sizeof(ftl_ctrl));
    for (i = 0; i < usersb; i++)
        ftl_map[i] = 3 + i;
    memset(&ftl_cxt, 0, sizeof(ftl_cxt));
    memset(ftl_cxt.field_3d8, 0xff, sizeof(ftl_cxt.field_3d8));
    for (n = 0; n < FTL_POOL_SIZE; n++)
        ftl_cxt.blockpool[n] = 3 + usersb + n;
    ftl_cxt.freecount = FTL_POOL_SIZE;
    ftl_cxt.usn = 0xfffffff0;
    ftl_nlogs = 0;
    memset(ftl_erasectr, 0, sizeof(ftl_erasectr));
    memset(ftl_readcount, 0, sizeof(ftl_readcount));
    memset(ftl_stats, 0xff, sizeof(ftl_stats));
    ftl_nmappages = (usersb * 2 + pagesize - 1) / pagesize;
    ftl_necpages = (nsuperblocks * 2 + pagesize - 1) / pagesize;
    ftl_nrcpages = ftl_necpages;
    ftl_nlogpages = (sbpages * FTL_MAX_LOGS * 2 + pagesize - 1) / pagesize;
    ftl_ctrl_idx = 0;
    ftl_ctrl_v = 0;
    ftl_mounted = true;
    ftl_writable = true;
    return ftl_commit() ? -3 : 0;
}

/* A medium from what nandcheck.py collected from a unit: report.txt for the
 * geometry, meta.bin for every page's spare record and pages.bin for every
 * page that is not user data. User data was not collected and reads as
 * zeros, which is enough to mount, resolve every logical page and write. */
static int open_collected(const char *dir)
{
    char path[512], line[128];
    unsigned banks = 0, blocks = 0, ppb = 0, pagesize = 0, userblocks = 0;
    unsigned mode = 0, planes = 0, vflspares = 0;
    size_t npages, g;
    uint32_t rec[2];
    FILE *f;

    snprintf(path, sizeof(path), "%s/report.txt", dir);
    if (!(f = fopen(path, "r")))
        return -1;
    while (fgets(line, sizeof(line), f))
    {
        sscanf(line, "banks %u", &banks);
        sscanf(line, "blocks %u", &blocks);
        sscanf(line, "ppb %u", &ppb);
        sscanf(line, "pagesize %u", &pagesize);
        sscanf(line, "userblocks %u", &userblocks);
        sscanf(line, "mode %u", &mode);
        sscanf(line, "planes %u", &planes);
        sscanf(line, "vflspares %u", &vflspares);
    }
    fclose(f);
    if (!banks || !blocks || !ppb || !pagesize || !planes)
        return -1;
    mock_geo.banks = banks;
    mock_geo.blocks = blocks;
    mock_geo.pagesperblock = ppb;
    mock_geo.pagesize = pagesize;
    mock_geo.userblocks = userblocks;
    mock_geo.planes = planes;
    mock_geo.vflspares = vflspares;
    mock_geo.mode = mode;
    mock_geo.twoplane = mode == 8;
    mock_geo.layout = mode == 1 ? NAND_LAYOUT_SINGLE
                    : mode == 3 || mode == 8 ? NAND_LAYOUT_ADJACENT
                    : mode == 2 || mode == 9 || mode == 12 ? NAND_LAYOUT_HALVES
                    : mode == 4 ? NAND_LAYOUT_BOTH
                    : mode == 13 ? NAND_LAYOUT_SPLIT13 : NAND_LAYOUT_UNKNOWN;
    mock_geo.validated = true;          /* a host copy: writes are safe */
    mock_open_blank();
    npages = (size_t)banks * blocks * ppb;

    snprintf(path, sizeof(path), "%s/meta.bin", dir);
    if (!(f = fopen(path, "rb")) || fread(mock_meta, 16, npages, f) != npages)
        return -1;
    fclose(f);
    snprintf(path, sizeof(path), "%s/pages.bin", dir);
    if (!(f = fopen(path, "rb")))
        return -1;
    while (fread(rec, 4, 2, f) == 2)
    {
        g = (size_t)rec[0] * blocks * ppb + rec[1];
        if (g >= npages
            || fread(mock_data + g * pagesize, 1, pagesize, f) != pagesize)
            break;
    }
    fclose(f);
    return 0;
}

/* Open the medium a test names: a dump directory, or "blank:" and a preset
 * geometry, formatted with ftl_hook_format(). The presets are rows of
 * Apple's chip table, with the reserved count nand-nano3g.c derives. */
int ftl_hook_open(const char *spec)
{
    static const struct
    {
        const char *name;
        unsigned banks, blocks, pagesize, userblocks, mode, planes, layout;
    } presets[] =
    {
        /* A514D3AD x 4, 4GB */
        { "2k4",   4, 4096, 2048, 3872,  8, 2, NAND_LAYOUT_ADJACENT },
        /* A555D5AD x 4, 8GB */
        { "2k8g",  4, 8192, 2048, 7744,  8, 2, NAND_LAYOUT_ADJACENT },
        /* B614D5EC x 2, 4GB */
        { "4k2",   2, 4096, 4096, 3872,  8, 2, NAND_LAYOUT_ADJACENT },
        /* 2555D5EC x 4 and A5D5D589 x 4, 8GB */
        { "halves", 4, 8192, 2048, 7744, 9, 2, NAND_LAYOUT_HALVES },
        /* A5D5D589 x 2, 4GB */
        { "both",  2, 8192, 2048, 7744,  4, 4, NAND_LAYOUT_BOTH },
        /* B614D5EC x 4, 8GB */
        { "single4k", 4, 4096, 4096, 3872, 1, 1, NAND_LAYOUT_SINGLE },
        /* A585D598 x 4, 8GB */
        { "split13", 4, 8320, 2048, 7744, 13, 2, NAND_LAYOUT_SPLIT13 },
    };
    unsigned i;

    if (!strncmp(spec, "collected:", 10))
        return open_collected(spec + 10);
    if (!strncmp(spec, "halves:", 7))
    {
        mock_open_layout(spec + 7, "halves");
        return 0;
    }
    if (!strncmp(spec, "both:", 5))
    {
        mock_open_layout(spec + 5, "both");
        return 0;
    }
    if (strncmp(spec, "blank:", 6))
    {
        mock_open(spec);
        return 0;
    }
    for (i = 0; i < ARRAYLEN(presets); i++)
        if (!strcmp(spec + 6, presets[i].name))
            break;
    if (i == ARRAYLEN(presets))
        return -1;
    mock_geo.banks = presets[i].banks;
    mock_geo.blocks = presets[i].blocks;
    mock_geo.pagesperblock = 128;
    mock_geo.planes = presets[i].planes;
    mock_geo.userblocks = presets[i].userblocks;
    mock_geo.vflspares = (presets[i].blocks - presets[i].userblocks)
                       / presets[i].planes - 23;
    mock_geo.twoplane = presets[i].mode == 8;
    mock_geo.pagesize = presets[i].pagesize;
    mock_geo.mode = presets[i].mode;
    mock_geo.layout = presets[i].layout;
    mock_geo.validated = true;
    mock_open_blank();
    return ftl_hook_format();
}

/* The unit a physical block belongs to under the mounted layout, spares
 * included, for tests that order pages as a superblock does; -1 if none */
int ftl_hook_block_unit(uint32_t block)
{
    static int16_t unit_of[8320];
    static const struct nand_geometry *built;
    uint32_t u, v;

    if (!geo || block >= geo->blocks || geo->blocks > ARRAYLEN(unit_of))
        return -1;
    if (built != geo)
    {
        for (v = 0; v < ARRAYLEN(unit_of); v++)
            unit_of[v] = -1;
        for (u = 0; u < geo->planes; u++)
            for (v = 0; v < geo->blocks / geo->planes; v++)
                unit_of[ftl_unit_block(u, v)] = u;
        built = geo;
    }
    return unit_of[block];
}

/* A bad spare followed by a replaced block in unit 0's table, as a spare
 * that fails its erase and the next remap leave it: mark the first free
 * slot bad, then remap vblock onto the following one */
int ftl_hook_bad_spare_then_remap(uint32_t bank, uint32_t vblock)
{
    struct ftl_vfl_cxt *cxt = &ftl_vfl_cxt[bank];
    uint32_t k;

    for (k = 0; k < geo->vflspares; k++)
        if (cxt->remap[k] == FTL_VFL_FREE)
            break;
    if (k == geo->vflspares)
        return -1;
    cxt->remap[k] = FTL_VFL_BAD;
    cxt->badcount++;
    return ftl_vfl_remap_block(bank, 0, vblock);
}

/* Put the block unit's vblock currently maps to on bank's pending-bad
 * list and commit, as Apple's firmware does after a failed read or
 * write; then erase it through the VFL, which must replace it first.
 * Returns the pending count left and whether the block moved. */
int ftl_hook_pending_erase(uint32_t bank, uint32_t unit, uint32_t vblock,
                           bool *moved)
{
    struct ftl_vfl_cxt *cxt = &ftl_vfl_cxt[bank];
    uint32_t before = ftl_vfl_phys_block(bank, unit, vblock);

    cxt->pending[cxt->pendingcount++] = before;
    if (ftl_vfl_commit_cxt(bank) || ftl_vfl_erase(bank, unit, vblock))
        return -1;
    *moved = ftl_vfl_phys_block(bank, unit, vblock) != before;
    return cxt->pendingcount;
}
