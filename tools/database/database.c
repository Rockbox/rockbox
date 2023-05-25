/* A _very_ skeleton file to demonstrate building tagcache db on host. */

#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

#include "config.h"
#include "tagcache.h"
#include "dir.h"

/* This is meant to be run on the root of the dap. it'll put the db files into
 * a .rockbox subdir */

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    errno = 0;
    if (mkdir(ROCKBOX_DIR) == -1 && errno != EEXIST)
        return 1;

    /* / is actually ., will get translated in io.c
     * (with the help of sim_root_dir below */
    const char *paths[] = { "/", NULL };
    tagcache_init();
    do_tagcache_build(paths);
    tagcache_reverse_scan();

    return 0;
}
