#include "plugin.h"

#include "SDL.h"
#include "SDL_video.h"

enum plugin_status plugin_start(const void *param)
{
    (void) param;
    size_t sz;
    void *pluginbuf = rb->plugin_get_buffer(&sz);
    init_memory_pool(sz, pluginbuf);

    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        rb->splash(HZ*2, SDL_GetError());
        return PLUGIN_ERROR;
    }

    rb->sleep(HZ*10);

    return PLUGIN_OK;
}
