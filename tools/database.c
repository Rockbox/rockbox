/* A _very_ skeleton file to demonstrate building tagcache db on host. */

#include <stdio.h>
#include "tagcache.h"

int main(int argc, char **argv)
{
    tagcache_init();
    tagcache_build("/export/stuff/mp3");
    tagcache_reverse_scan();
    
    return 0;
}

