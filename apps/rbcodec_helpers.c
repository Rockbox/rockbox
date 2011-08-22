#include "core_alloc.h"
#include "dsp.h"
#include "rbcodecplatform.h"
#include "sound.h"

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

#if CONFIG_CODEC == SWCODEC
/* Hook back from firmware/ part of audio, which can't/shouldn't call apps/
 * code directly.
 */
int dsp_callback(int msg, intptr_t param)
{
#ifdef HAVE_SW_TONE_CONTROLS
    static int bass, treble;
#endif
    switch (msg)
    {
#ifdef HAVE_SW_TONE_CONTROLS
    case DSP_CALLBACK_SET_PRESCALE:
        set_tone_controls(bass, treble, param);
        break;
    /* prescaler is always set after calling any of these, so we wait with
     * calculating coefs until the above case is hit.
     */
    case DSP_CALLBACK_SET_BASS:
        bass = param;
        break;
    case DSP_CALLBACK_SET_TREBLE:
        treble = param;
        break;
#ifdef HAVE_SW_VOLUME_CONTROL
    case DSP_CALLBACK_SET_SW_VOLUME:
        rbcodec_dsp_set_volume(global_settings.volume);
        break;
#endif
#endif
    case DSP_CALLBACK_SET_CHANNEL_CONFIG:
        dsp_set_channel_config(param);
        break;
    case DSP_CALLBACK_SET_STEREO_WIDTH:
        dsp_set_stereo_width(param);
        break;
    default:
        break;
    }
    return 0;
}
#endif
