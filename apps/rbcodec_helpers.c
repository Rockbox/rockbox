#include "core_alloc.h"
#include "rbcodecplatform.h"

struct alloc_info {
    void (*move)(void *from, void *to);
    int handle;
};

static int move_callback(int handle, void *from, void *to)
{
    (void)handle;
    struct alloc_info *from_info = (struct alloc_info *)from;
    struct alloc_info *to_info = (struct alloc_info *)to;
    from = from_info + 1;
    to = to_info + 1;
    from_info->move(from, to);
    return BUFLIB_CB_OK;
}

static struct buflib_callbacks ops = {
    .move_callback = move_callback,
    .shrink_callback = NULL,
};

void *rbcodec_alloc(size_t size, void (*move)(void *from, void *to))
{
    int handle = core_alloc_ex("rbcodec_alloc", size, &ops);
    if (handle >= 0) {
        struct alloc_info *info = core_get_data(handle);
        info->move = move;
        info->handle = handle;
        return info + 1;
    } else {
        return 0;
    }
}

void rbcodec_free(void *ptr)
{
    if (ptr) {
        struct alloc_info *info = ((struct alloc_info *)ptr) - 1;
        core_free(info->handle);
    }
}
