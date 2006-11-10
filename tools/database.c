/* A _very_ skeleton file to demonstrate building tagcache db on host. */

#include <stdio.h>
#include "tagcache.h"

int main(int argc, char **argv)
{
    tagcache_init();
    build_tagcache("/export/stuff/mp3");
    
    return 0;
}

