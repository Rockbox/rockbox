/*
 * malloc.c: safe wrappers around malloc, realloc, free, strdup
 */

#include <stdlib.h>
#include <string.h>
#include "src/puzzles.h"

/*
 * smalloc should guarantee to return a useful pointer - Halibut
 * can do nothing except die when it's out of memory anyway.
 */

int allocs = 0;
int frees = 0;

/* We don't load as an overlay anymore, so the audiobuf should always
 * be available. */
bool audiobuf_available = true;

static bool grab_audiobuf(void)
{
    if(!audiobuf_available)
        return false;

    if(rb->audio_status())
        rb->audio_stop();

    size_t sz;

    void *audiobuf = rb->plugin_get_audio_buffer(&sz);
    extern char *giant_buffer;

#if 0
    /* Try aligning if tlsf crashes in add_new_area().  This is
     * disabled now since things seem to work without it. */
    void *old = audiobuf;

    /* I'm sorry. */
    audiobuf = (void*)((int) audiobuf & ~0xff);
    audiobuf += 0x100;

    sz -= audiobuf - old;
#endif

    add_new_area(audiobuf, sz, giant_buffer);
    audiobuf_available = false;
    return true;
}

void *smalloc(size_t size) {
    void *p;
    p = malloc(size);
    //LOGF("allocs: %d", ++allocs);
    if (!p)
    {
        if(grab_audiobuf())
            return smalloc(size);
        fatal("out of memory");
    }
    return p;
}

/*
 * sfree should guaranteeably deal gracefully with freeing NULL
 */
void sfree(void *p) {
    if (p) {
        ++frees;
        //LOGF("frees: %d, total outstanding: %d", frees, allocs - frees);
        free(p);
    }
}

/*
 * srealloc should guaranteeably be able to realloc NULL
 */
void *srealloc(void *p, size_t size) {
    void *q;
    if (p) {
	q = realloc(p, size);
    } else {
        //LOGF("allocs: %d", ++allocs);
	q = malloc(size);
    }
    if (!q)
    {
        if(grab_audiobuf())
            return srealloc(p, size);
        fatal("out of memory");
    }
    return q;
}

/*
 * dupstr is like strdup, but with the never-return-NULL property
 * of smalloc (and also reliably defined in all environments :-)
 */
char *dupstr(const char *s) {
    char *r = smalloc(1+strlen(s));
    strcpy(r,s);
    return r;
}
