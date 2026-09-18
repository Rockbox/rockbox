/* Tests for the mock NAND itself: erase/program semantics, the violation
 * counters, and - most important - that writes never reach the dump files.
 * Everything downstream (the FTL write path) rests on these holding.
 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "nand-target.h"
#include "mock_nand.h"

#define BANKS   4
#define BLOCKS  4096
#define PPB     128

static int fails;
static const char *dumpdir;

static void check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        fails++;
}

static int all_bytes(const void *p, size_t n, uint8_t v)
{
    const uint8_t *b = p;
    while (n--)
        if (*b++ != v)
            return 0;
    return 1;
}

/* Read straight from the file on disk, bypassing the mapping */
static int file_page(uint32_t bank, uint32_t page, uint8_t *out)
{
    char path[512];
    size_t g = (size_t)bank * BLOCKS * PPB + page;
    int fd, ok;

    snprintf(path, sizeof(path), "%s/nand_data_4banks.bin", dumpdir);
    fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;
    ok = pread(fd, out, NAND_PAGE_SIZE, (off_t)g * NAND_PAGE_SIZE)
         == NAND_PAGE_SIZE;
    close(fd);
    return ok ? 0 : -1;
}

int main(int argc, char **argv)
{
    static uint8_t buf[NAND_PAGE_SIZE], buf2[NAND_PAGE_SIZE];
    static uint8_t orig[NAND_PAGE_SIZE], pattern[NAND_PAGE_SIZE];
    uint32_t meta[NAND_META_WORDS], meta2[NAND_META_WORDS];
    uint32_t bank = 0, block = 0, base;
    int rc, i;

    dumpdir = argc > 1 ? argv[1] : "../n3g-dump";
    mock_open(dumpdir);

    /* Find a block whose page 0 is written, to test erase against */
    for (block = 0; block < BLOCKS; block++)
        if (nand_read_page(bank, block * PPB, buf, meta) == NAND_ECC_CORRECTED)
            break;
    check("found a written block to work on", block < BLOCKS);
    if (block >= BLOCKS)
        return 1;
    base = block * PPB;
    memcpy(orig, buf, NAND_PAGE_SIZE);
    check("a written page reads CORRECTED and matches the dump",
          !file_page(bank, base, buf2) && !memcmp(orig, buf2, NAND_PAGE_SIZE));

    /* Erase */
    mock_reset_stats();
    check("erase returns 0", nand_erase_block(bank, block) == 0);
    rc = nand_read_page(bank, base, buf, meta);
    check("an erased page reads CLEAN, all 0xff, meta all 0xff",
          rc == NAND_ECC_CLEAN && all_bytes(buf, NAND_PAGE_SIZE, 0xff)
          && all_bytes(meta, sizeof(meta), 0xff));
    rc = nand_read_page(bank, base + PPB - 1, buf, meta);
    check("erase covers the whole block",
          rc == NAND_ECC_CLEAN && all_bytes(buf, NAND_PAGE_SIZE, 0xff));
    check("erase counted", mock_erases == 1);

    /* Program */
    for (i = 0; i < NAND_PAGE_SIZE; i++)
        pattern[i] = (uint8_t)(i * 7 + 1);
    meta[0] = 0x12345678; meta[1] = 0x9abcdef0; meta[2] = 0x0000ff40;
    check("program returns 0",
          nand_write_page(bank, base, pattern, meta) == 0);
    rc = nand_read_page(bank, base, buf, meta2);
    check("a programmed page reads back exactly, as CORRECTED",
          rc == NAND_ECC_CORRECTED && !memcmp(buf, pattern, NAND_PAGE_SIZE)
          && !memcmp(meta2, meta, sizeof(meta)));
    check("clean program raises no violations",
          mock_reprogram == 0 && mock_nonsequential == 0);

    /* Program only clears bits */
    memset(buf2, 0x0f, NAND_PAGE_SIZE);
    check("reprogramming returns 0",
          nand_write_page(bank, base, buf2, meta) == 0);
    rc = nand_read_page(bank, base, buf, meta2);
    for (i = 0; i < NAND_PAGE_SIZE; i++)
        if (buf[i] != (uint8_t)(pattern[i] & 0x0f))
            break;
    check("programming only clears bits (reads old AND new)",
          i == NAND_PAGE_SIZE);
    check("reprogramming a written page is counted", mock_reprogram == 1);

    /* Out-of-order programming within a block */
    mock_reset_stats();
    nand_erase_block(bank, block);
    nand_write_page(bank, base + 2, pattern, meta);
    check("skipping a page within a block is counted",
          mock_nonsequential == 1);
    mock_reset_stats();
    nand_erase_block(bank, block);
    nand_write_page(bank, base, pattern, meta);
    nand_write_page(bank, base + 1, pattern, meta);
    check("sequential programming raises no violation",
          mock_nonsequential == 0 && mock_reprogram == 0);

    /* Bounds */
    check("out-of-range bank fails",
          nand_read_page(BANKS, 0, buf, NULL) == -1
          && nand_write_page(BANKS, 0, pattern, meta) == -1
          && nand_erase_block(BANKS, 0) == -1);
    check("out-of-range page/block fails",
          nand_read_page(0, BLOCKS * PPB, buf, NULL) == -1
          && nand_write_page(0, BLOCKS * PPB, pattern, meta) == -1
          && nand_erase_block(0, BLOCKS) == -1);

    /* The whole point: none of that reached the file */
    check("the dump file on disk is untouched",
          !file_page(bank, base, buf2) && !memcmp(orig, buf2, NAND_PAGE_SIZE));

    printf(fails ? "SOME CHECKS FAILED\n" : "ALL PASS\n");
    return fails ? 1 : 0;
}
