/* The mount's chip checks: a row that is not validated mounts read-only,
 * a row whose reserved-block count does not match the VFL context is
 * refused, and so are unknown layouts and page sizes. */
#include <stdio.h>
#include <string.h>
#include "nand-target.h"
#include "ftl-target.h"
#include "mock_nand.h"

static int fails;

static void check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        fails++;
}

int main(int argc, char **argv)
{
    static uint8_t buf[4 * NAND_PAGE_SIZE], orig[4 * NAND_PAGE_SIZE];
    const struct nand_geometry good = mock_geo;
    int rc;

    mock_open(argc > 1 ? argv[1] : "../n3g-dump");

    mock_geo.validated = false;
    check("unvalidated row mounts", ftl_init() == 0);
    check("... read-only", !ftl_hook_writeable());
    check("... reads", ftl_read(1000 * 1024, 4, orig) == 0);
    memset(buf, 0x5a, sizeof(buf));
    check("... refuses a write", ftl_write(1000 * 1024, 4, buf) != 0);
    check("... sync does nothing", ftl_sync() == 0);
    check("... no program or erase reached the NAND",
          mock_writes == 0 && mock_erases == 0);
    check("... data unchanged", ftl_read(1000 * 1024, 4, buf) == 0
                                && !memcmp(buf, orig, sizeof(buf)));
    mock_geo = good;

    mock_geo.vflspares = 90;
    rc = ftl_init();
    printf("     vflspares 90: %d\n", rc);
    check("one reserved block too many is refused", rc == -5);
    mock_geo.vflspares = 88;
    rc = ftl_init();
    printf("     vflspares 88: %d\n", rc);
    check("one reserved block too few is refused", rc == -5);
    mock_geo = good;

    mock_geo.layout = NAND_LAYOUT_UNKNOWN;
    check("an unknown layout is refused", ftl_init() == -4);
    mock_geo = good;
    mock_geo.pagesize = 8192;
    check("an unsupported page size is refused", ftl_init() == -4);
    mock_geo = good;

    check("the real row mounts writable",
          ftl_init() == 0 && ftl_hook_writeable());

    puts(fails ? "SOME FAILED" : "ALL PASS");
    return fails != 0;
}
