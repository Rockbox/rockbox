#include "plugin.h"
#define FAVORITES_FILE "/favorites.m3u"

static struct plugin_api* rb;

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    struct mp3entry* id3;
    char track_path[MAX_PATH+1];
    int fd, seek, result, len;
    
    /* this macro should be called as the first thing you do in the plugin.
       it test that the api version and model the plugin was compiled for
       matches the machine it is running on */
    TEST_PLUGIN_API(api);

    /* if you don't use the parameter, you can do like
       this to avoid the compiler warning about it */
    (void)parameter;

    rb = api;

    id3 = rb->mpeg_current_track();
    if (!id3)
        return PLUGIN_ERROR;
    
    fd = rb->open(FAVORITES_FILE, O_WRONLY);

    // creat the file if it does not return on open.
    if (fd < 0)
        fd = rb->creat(FAVORITES_FILE, 0);

    if (fd > 0) {
        rb->strcpy(track_path, id3->path);
        len = rb->strlen(track_path);

        // seek to the end of file
        seek = rb->lseek(fd, 0, SEEK_END);
        // append the current mp3 path
        track_path[len] = '\n';
        result = rb->write(fd, track_path, len + 1);
        track_path[len] = '\0';
        rb->close(fd);
    }

    rb->splash(HZ*2, 0, true, "Saved Favorite");

    return PLUGIN_OK;
}


