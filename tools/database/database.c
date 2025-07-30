/* A _very_ skeleton file to demonstrate building tagcache db on host. */

#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"
#include "tagcache.h"
#include "dir.h"

/* This is meant to be run on the root of the dap. it'll put the db files into
 * a .rockbox subdir */

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    fprintf(stderr, "Rockbox database tool for '%s'\n\n", TARGET_NAME);

    DIR* rbdir = opendir(ROCKBOX_DIR);
    if (!rbdir) {
        fprintf(stderr, "Unable to find the '%s' directory!\n", ROCKBOX_DIR);
        fprintf(stderr, "This needs to be executed in your DAP's top-level directory!\n\n");
        return 1;
    }
    closedir(rbdir);

    /* / is actually ., will get translated in io.c
     * (with the help of sim_root_dir below */
    const char *paths[] = { "/", NULL };
    tagcache_init();

    fprintf(stderr, "Scanning files (may take some time)...");

    do_tagcache_build(paths);
    tagcache_reverse_scan();

    fprintf(stderr, "...done!\n");

    return 0;
}


/* needed for io.c */
const char *sim_root_dir = ".";

/* stubs to avoid including thread-sdl.c */
#include "kernel.h"
void mutex_init(struct mutex *m)
{
    (void)m;
}

void mutex_lock(struct mutex *m)
{
    (void)m;
}

void mutex_unlock(struct mutex *m)
{
    (void)m;
}

void sim_thread_lock(void *me)
{
    (void)me;
}

void * sim_thread_unlock(void)
{
    return (void*)1;
}
