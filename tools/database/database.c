/* A _very_ skeleton file to demonstrate building tagcache db on host. */

#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include "tagcache.h"

int main(int argc, char **argv)
{
    tagcache_init();
    tagcache_build(".");
    tagcache_reverse_scan();
    
    return 0;
}

/* stub to avoid including all of apps/misc.c */
bool file_exists(const char *file)
{
    struct stat s;
    if (!stat(file, &s))
        return true;
    return false;
}

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
    
