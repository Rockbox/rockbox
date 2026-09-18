/* Replay a block-layer write trace through the FTL on the mock NAND and
 * report what it cost. The trace is Linux's block_rq_issue tracepoint from a
 * guest writing to a disk with 4096-byte logical sectors, so each 512-byte
 * trace sector is a quarter of an FTL sector. Writes are split as Rockbox's
 * USB storage does, into calls of at most WRITE_BUFFER_SIZE (24 KB).
 *
 *   replay TRACE [DUMPDIR]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nand-target.h"
#include "ftl-target.h"
#include "mock_nand.h"

#define CHUNK_SECTORS   (24 * 1024 / NAND_PAGE_SIZE)   /* 11: 24 KB, whole */
#define SEC_PER_4K      2

/* Counters an instrumented FTL build may provide */
extern unsigned long st_merge __attribute__((weak)),
    st_merge_wear __attribute__((weak)), st_merge_full __attribute__((weak)),
    st_seqadopt __attribute__((weak)), st_seqfill __attribute__((weak)),
    st_sync __attribute__((weak)), st_bound_logs __attribute__((weak)),
    st_bound_pool __attribute__((weak)), st_append __attribute__((weak)),
    st_log_alloc __attribute__((weak));

static void report(const char *phase, unsigned long hostpages,
                   unsigned long calls)
{
    unsigned long data = mock_writes_by_type[0x40] + mock_writes_by_type[0x41];
    unsigned long ctrl = mock_writes - data;

    printf("%-8s host pages %7lu in %6lu calls | programs %7lu "
           "(data %7lu = %.2fx host, ctrl %5lu) reads %7lu erases %5lu | "
           "commits %lu, marks %lu\n",
           phase, hostpages, calls, mock_writes, data,
           hostpages ? (double)data / hostpages : 0.0, ctrl, mock_reads,
           mock_erases, mock_writes_by_type[0x43], mock_writes_by_type[0x4f]);
    printf("         program runs %lu (%.1f pages each), two-plane %lu "
           "(%.0f%% of pages), bad pairs %lu\n", mock_runs,
           mock_runs ? (double)mock_run_pages / mock_runs : 0.0,
           mock_twoplane_runs,
           mock_run_pages ? 100.0 * mock_twoplane_pages / mock_run_pages : 0.0,
           mock_bad_twoplane);
    printf("         read runs %lu (%.1f pages each)\n", mock_read_runs,
           mock_read_runs ? (double)mock_read_run_pages / mock_read_runs : 0.0);
    if (&st_merge)
    {
        printf("         appends %lu, logs opened %lu | syncs %lu (bound: logs %lu, "
               "pool %lu) | merges %lu (whole-block %lu, wear %lu, folds %lu) "
               "| sequential adoptions %lu filling %lu pages\n",
               st_append, st_log_alloc, st_sync, st_bound_logs, st_bound_pool,
               st_merge, st_merge_full, st_merge_wear,
               st_merge - st_merge_full - st_merge_wear, st_seqadopt, st_seqfill);
        st_append = st_log_alloc = st_sync = st_bound_logs = st_bound_pool = 0;
        st_merge = st_merge_full = st_merge_wear = st_seqadopt = st_seqfill = 0;
    }
}

int main(int argc, char **argv)
{
    static uint8_t buf[32 * NAND_PAGE_SIZE];
    char line[512];
    FILE *f;
    unsigned long hostpages = 0, calls = 0, reqs = 0;
    const char *phase = "copy";

    if (argc < 2)
    {
        fprintf(stderr, "usage: replay TRACE [DUMPDIR]\n");
        return 2;
    }
    f = fopen(argv[1], "r");
    if (!f)
    {
        perror(argv[1]);
        return 1;
    }
    mock_open(argc > 2 ? argv[2] : "../n3g-dump");
    if (ftl_init() || ftl_sync() || ftl_init())
    {
        puts("mount/settle failed");
        return 1;
    }
    mock_reset_stats();
    memset(buf, 0xa5, sizeof(buf));

    while (fgets(line, sizeof(line), f))
    {
        char *p, rwbs[8];
        unsigned long long sector;
        unsigned long nr, bytes;
        uint32_t lpn, n, done;

        if (strstr(line, "MARK COPY_DONE"))
        {
            if (ftl_sync())
                puts("sync failed");
            report(phase, hostpages, calls);
            mock_reset_stats();
            hostpages = calls = 0;
            phase = "delete";
            continue;
        }
        if (strstr(line, "MARK DELETE_DONE"))
            break;
        p = strstr(line, "block_rq_issue: ");
        if (!p)
            continue;
        if (sscanf(p, "block_rq_issue: %*s %7s %lu () %llu + %lu",
                   rwbs, &bytes, &sector, &nr) != 4)
            continue;
        if (!strchr(rwbs, 'W') || !nr)
            continue;
        reqs++;
        lpn = sector / 4;
        n = nr / 4;
        for (done = 0; done < n; done += CHUNK_SECTORS / SEC_PER_4K * SEC_PER_4K)
        {
            uint32_t c = n - done;

            if (c > CHUNK_SECTORS / SEC_PER_4K * SEC_PER_4K)
                c = CHUNK_SECTORS / SEC_PER_4K * SEC_PER_4K;
            if (ftl_write(lpn + done, c, buf))
            {
                printf("ftl_write(%u, %u) failed\n", lpn + done, c);
                return 1;
            }
            calls++;
            hostpages += c;
        }
    }
    if (ftl_sync())
        puts("final sync failed");
    report(phase, hostpages, calls);
    printf("%lu write requests; NAND rule violations: reprogram %lu, "
           "nonsequential %lu\n", reqs, mock_reprogram, mock_nonsequential);
    fclose(f);
    return 0;
}
