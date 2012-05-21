
#include "config.h"
#include <string.h>
#include "system.h"
#include "core_alloc.h"
#include "buflib.h"

/* not static so it can be discovered by core_get_data() */
struct buflib_context core_ctx;

/* defined in linker script */
#if (CONFIG_PLATFORM & PLATFORM_NATIVE) && !defined(__PCTOOL__)
#if defined(IPOD_VIDEO) && !defined(BOOTLOADER)
extern unsigned char *audiobufend_lds[];
unsigned char *audiobufend;
#else /* !IPOD_VIDEO */
extern unsigned char audiobufend[];
#endif
/* defined in linker script */
extern unsigned char audiobuffer[];
#else /* PLATFORM_HOSTED */
unsigned char audiobuffer[(MEMORYSIZE*1024-256)*1024];
unsigned char *audiobufend = audiobuffer + sizeof(audiobuffer);
extern unsigned char *audiobufend;
#endif

static struct mutex core_mtx SHAREDBSS_ATTR;
static void core_mutex_lock(void)
{
    mutex_lock(&core_mtx);
}

static void core_mutex_unlock(void)
{
    mutex_unlock(&core_mtx);
}

/* debug test alloc */
static int test_alloc;
void core_allocator_init(void)
{
    unsigned char *start = ALIGN_UP(audiobuffer, sizeof(intptr_t));

#if defined(IPOD_VIDEO) && !defined(BOOTLOADER) && !defined(SIMULATOR)
    audiobufend=(unsigned char *)audiobufend_lds;
    if(MEMORYSIZE==64 && probed_ramsize!=64)
    {
        audiobufend -= (32<<20);
    }
#endif

    mutex_init(&core_mtx);

    buflib_init(&core_ctx, start, audiobufend - start);

    test_alloc = core_alloc("test", 112);
}

bool core_test_free(void)
{
    bool ret = test_alloc > 0;
    if (ret)
        test_alloc = core_free(test_alloc);

    return ret;
}

int core_alloc(const char* name, size_t size)
{
    core_mutex_lock();
    int handle = buflib_alloc_ex(&core_ctx, size, name, NULL);
    core_mutex_unlock();
    return handle;
}

int core_alloc_ex(const char* name, size_t size, struct buflib_callbacks *ops)
{
    core_mutex_lock();
    int handle = buflib_alloc_ex(&core_ctx, size, name, ops);
    core_mutex_unlock();
    return handle;
}

size_t core_available(void)
{
    core_mutex_lock();
    size_t size = buflib_available(&core_ctx);
    core_mutex_unlock();
    return size;
}

int core_free(int handle)
{
    /* Doesn't currently do callbacks; no lock */
    return buflib_free(&core_ctx, handle);
}

int core_alloc_maximum(const char* name, size_t *size, struct buflib_callbacks *ops)
{
    core_mutex_lock();
    int handle = buflib_alloc_maximum(&core_ctx, name, size, ops);
    core_mutex_unlock();
    return handle;
}

bool core_shrink(int handle, void* new_start, size_t new_size)
{
    /* Doesn't currently do callbacks; no lock */
    return buflib_shrink(&core_ctx, handle, new_start, new_size);
}

int core_get_num_blocks(void)
{
    return buflib_get_num_blocks(&core_ctx);
}

void core_print_block_at(int block_num, char* buf, size_t bufsize)
{
    buflib_print_block_at(&core_ctx, block_num, buf, bufsize);
}
