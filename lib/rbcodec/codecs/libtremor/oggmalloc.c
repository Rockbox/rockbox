#include "os_types.h"
#include <tlsf.h>

#if defined(CPU_ARM) || defined(CPU_COLDFIRE) || defined(CPU_MIPS)
#include <setjmp.h>
extern jmp_buf rb_jump_buf;
#define LONGJMP(x)  longjmp(rb_jump_buf, x)
#elif defined(SIMULATOR)
#define LONGJMP(x)  do { DEBUGF("Vorbis: allocation failed!\n"); return NULL; } while (false)
#else
#define LONGJMP(x)  return NULL
#endif

void ogg_malloc_init(void)
{
    size_t bufsize;
    void* buf = ci->codec_get_buffer(&bufsize);
    init_memory_pool(bufsize, buf);
}

void ogg_malloc_destroy()
{
    size_t bufsize;
    void* buf = ci->codec_get_buffer(&bufsize);
    destroy_memory_pool(buf);
}

void *ogg_malloc(size_t size)
{
    void* x = tlsf_malloc(size);

    if (x == NULL)
        LONGJMP(1);
    
    return x;
}

void *ogg_calloc(size_t nmemb, size_t size)
{
    void *x = tlsf_calloc(nmemb, size);

    if (x == NULL)
        LONGJMP(1);
    
    return x;
}

void *ogg_realloc(void *ptr, size_t size)
{
    void *x = tlsf_realloc(ptr, size);

    if (x == NULL)
        LONGJMP(1);
    
    return x;
}

void ogg_free(void* ptr)
{
    tlsf_free(ptr);
}

#ifdef TREMOR_USE_IRAM
/* Allocate IRAM buffer */
static unsigned char iram_buff[IRAM_IBSS_SIZE] IBSS_ATTR MEM_ALIGN_ATTR;
static size_t iram_remain;

void iram_malloc_init(void){
    iram_remain=IRAM_IBSS_SIZE;
}

void *iram_malloc(size_t size){
    void* x;

    /* always ensure alignment to CACHEALIGN_SIZE byte */
    size = (size + (CACHEALIGN_SIZE-1)) & ~(CACHEALIGN_SIZE-1);

    if(size>iram_remain)
      return NULL;

    x = &iram_buff[IRAM_IBSS_SIZE-iram_remain];
    iram_remain-=size;

    return x;
}
#endif
