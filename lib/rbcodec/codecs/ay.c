
/* Ripped off from Game_Music_Emu 0.5.2. http://www.slack.net/~ant/ */

#include <codecs/lib/codeclib.h>
#include "libgme/ay_emu.h" 

CODEC_HEADER

/* Maximum number of bytes to process in one iteration */
#define CHUNK_SIZE (1024*2)

static int16_t samples[CHUNK_SIZE] IBSS_ATTR;
static struct Ay_Emu ay_emu;

/****************** rockbox interface ******************/

static void set_codec_track(int t, int multitrack) {
    Ay_start_track(&ay_emu, t); 

    /* for loop mode we disable track limits */
    if (!ci->loop_track()) {
        Track_set_fade(&ay_emu, Track_get_length( &ay_emu, t ) - 4000, 4000);
    }
    if (multitrack) ci->set_elapsed(t*1000); /* t is track no to display */
    else ci->set_elapsed(0);
}

/* this is the codec entry point */
enum codec_status codec_main(enum codec_entry_call_reason reason)
{
    if (reason == CODEC_LOAD) {
        /* we only render 16 bits */
        ci->configure(DSP_SET_SAMPLE_DEPTH, 16);

        /* 44 Khz, Interleaved stereo */
        ci->configure(DSP_SET_FREQUENCY, 44100);
        ci->configure(DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);

        Ay_init(&ay_emu);
        Ay_set_sample_rate(&ay_emu, 44100);
    }

    return CODEC_OK;
}

/* this is called for each file to process */
enum codec_status codec_run(void)
{
    blargg_err_t err;
    uint8_t *buf;
    size_t n;
    int track, is_multitrack;
    intptr_t param;
    uint32_t elapsed_time;

    /* reset values */
    track = is_multitrack = 0;
    elapsed_time = 0;

    DEBUGF("AY: next_track\n");
    if (codec_init()) {
        return CODEC_ERROR;
    }  

    codec_set_replaygain(ci->id3);
        
    /* Read the entire file */
    DEBUGF("AY: request file\n");
    ci->seek_buffer(0);
    buf = ci->request_buffer(&n, ci->filesize);
    if (!buf || n < (size_t)ci->filesize) {
        DEBUGF("AY: file load failed\n");
        return CODEC_ERROR;
    }
   
    if ((err = Ay_load_mem(&ay_emu, buf, ci->filesize))) {
        DEBUGF("AY: Ay_load_mem failed (%s)\n", err);
        return CODEC_ERROR;
    }

    /* Update internal track count */
    if (ay_emu.m3u.size > 0)
        ay_emu.track_count = ay_emu.m3u.size;

    /* Check if file has multiple tracks */
    if (ay_emu.track_count > 1) {
        is_multitrack = 1;
    }

next_track:
    set_codec_track(track, is_multitrack);

    /* The main decoder loop */
    while (1) {
        enum codec_command_action action = ci->get_command(&param);

        if (action == CODEC_ACTION_HALT)
            break;

        if (action == CODEC_ACTION_SEEK_TIME) {
            if (is_multitrack) {
                track = param/1000;
                ci->seek_complete();
                if (track >= ay_emu.track_count) break;
                goto next_track;
            }

            ci->set_elapsed(param);
            elapsed_time = param;
            Track_seek(&ay_emu, param);
            ci->seek_complete();
            
            /* Set fade again */
            if (!ci->loop_track()) {
                Track_set_fade(&ay_emu, Track_get_length( &ay_emu, track ) - 4000, 4000);
            }
        }

        /* Generate audio buffer */
        err = Ay_play(&ay_emu, CHUNK_SIZE, samples);
        if (err || Track_ended(&ay_emu)) {
            track++;
            if (track >= ay_emu.track_count) break;
            goto next_track;
        }

        ci->pcmbuf_insert(samples, NULL, CHUNK_SIZE >> 1);

        /* Set elapsed time for one track files */
        if (!is_multitrack) {
            elapsed_time += (CHUNK_SIZE / 2) * 10 / 441;
            ci->set_elapsed(elapsed_time);
        }
    }

    return CODEC_OK;
}
