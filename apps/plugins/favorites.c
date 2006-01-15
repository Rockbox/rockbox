#include "plugin.h"
#define FAVORITES_FILE "/favorites.m3u"

PLUGIN_HEADER

static struct plugin_api* rb;

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    struct mp3entry* id3;
    char track_path[MAX_PATH+1];
    int fd, result, len;
    
    rb = api;

    /* If we were passed a parameter, use that as the file name,
       else take the currently playing track */
    if(parameter) {
        rb->strncpy(track_path, parameter, MAX_PATH);
    } else {
        id3 = rb->audio_current_track();
        if (!id3) {
            rb->splash(HZ*2, true, "Nothing To Save");
            return PLUGIN_OK;
        }
        rb->strncpy(track_path, id3->path, MAX_PATH);
    }

    track_path[MAX_PATH] = 0;
    
    len = rb->strlen(track_path);

    fd = rb->open(FAVORITES_FILE, O_CREAT|O_WRONLY|O_APPEND);

    if (fd >= 0) {
        // append the current mp3 path
        track_path[len] = '\n';
        result = rb->write(fd, track_path, len + 1);
        track_path[len] = '\0';
        rb->close(fd);
    }

    rb->splash(HZ*2, true, "Saved Favorite");

    return PLUGIN_OK;
}
